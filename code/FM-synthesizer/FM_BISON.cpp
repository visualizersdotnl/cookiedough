
/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl
*/

// Shut it, MSVC.
#define _CRT_SECURE_NO_WARNINGS

// C++11
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <deque>

#include "FM_BISON.h"
#include "synth-midi.h"
#include "synth-parameters.h"
#include "synth-ringbuffer.h"
#include "synth-filter.h"
#include "synth-delayline.h"
#include "synth-math.h"
#include "synth-LUT.h"
#include "synth-formant.h"
#include "synth-DX-voice.h"

// Win32 MIDI input (M-AUDIO Oxygen 49 & Arturia BeatStep)
#include "Win-MIDI-in-Oxygen49.h"
#include "Win-MIDI-in-BeatStep.h"

// SDL2 audio output
#include "SDL2-audio-out.h"

namespace SFM
{
	/*
		Global sample counts.
	*/

 	static unsigned s_sampleCount = 0;
	static unsigned s_sampleOutCount = 0;

	/*
		Ring buffer.
	*/

	static std::mutex s_ringBufMutex;
	static FIFO s_ringBuf;

	/*
		State.

		May only be altered after acquiring lock.
	*/

	static std::mutex s_stateMutex;

	struct VoiceRequest
	{
		unsigned *pIndex;
		Waveform form;
		float frequency;
		float velocity;
	};

	struct VoiceReleaseRequest
	{
		unsigned index;
		float velocity;
	};

	// Voice req.
	static std::deque<VoiceRequest> s_voiceReq;
	static std::deque<VoiceReleaseRequest> s_voiceReleaseReq;

	// Parameters
	static Parameters s_parameters;

	// Voices
	static DX_Voice s_DXvoices[kMaxVoices];
	static unsigned s_active = 0;
	static ADSR s_voiceADSRs[kMaxVoices];

	// FX
	static ButterworthFilter s_butterFilters[kMaxVoices];
	static TeemuFilter s_teemuFilters[kMaxVoices];
	static ImprovedMOOGFilter s_improvedFilters[kMaxVoices];
	static DelayMatrix s_delayMatrix(kSampleRate/8); // Div. by multiples of 4 sounds OK
	static FormantShaper s_formantShapers[kMaxVoices];
	static LowpassFilter s_bassBoostLPF;
	
	/*
		Voice API.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, float frequency, float velocity)
	{
		SFM_ASSERT(true == InAudibleSpectrum(frequency));

		std::lock_guard<std::mutex> lock(s_stateMutex);
		
		VoiceRequest request;
		request.pIndex = pIndex;
		request.form = form;
		request.frequency = frequency;
		request.velocity = velocity;
		s_voiceReq.push_front(request);

		// At this point there is no voice yet
		if (nullptr != pIndex)
			*pIndex = -1;
	}

	void ReleaseVoice(unsigned index, float velocity)
	{
		SFM_ASSERT(-1 != index);

		std::lock_guard<std::mutex> lock(s_stateMutex);

		VoiceReleaseRequest request; 
		request.index = index;
		request.velocity = velocity;

		s_voiceReleaseReq.push_front(request);
	}

	/*
		Voice logic.
	*/

	static void InitializeDXVoice(const VoiceRequest &request, unsigned iVoice)
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		DX_Voice &voice = s_DXvoices[iVoice];
		
		const float goldenTen = kGoldenRatio*10.f;

		float frequency = request.frequency;
		
		// Randomize note frequency little if wanted (in (almost) entire cents)
		const float drift = kMaxNoteDrift*s_parameters.m_noteDrift;
		const int cents = int(ceilf(oscWhiteNoise()*drift));
		
		float jitter = 1.f;
		if (0 != cents)
			jitter = powf(2.f, (cents*0.01f)/12.f);
		
		FloatAssert(jitter);

		frequency *= jitter;

		// Each has a distinct effect (linear, exponential, inverse exponential)
		const float velocity       = request.velocity;
//		const float velocityExp    = velocity*velocity;
		const float velocityInvExp = Clamp(invsqrf(velocity));
		
		const bool  isWave = oscIsWavetable(request.form);

		// Carrier amplitude & frequency
		const float amplitude = velocity*kMaxVoiceAmp;
		const float carrierFreq = frequency;

		// Modulation ratio, frequency & index
		const float modRatio   = s_parameters.m_modRatioM;
		const float modFreq    = carrierFreq*modRatio;
		const float modIndex   = s_parameters.m_modIndex*velocityInvExp;
		const float brightness = s_parameters.m_modBrightness;

		// Set up algorithm (hardcoded, could consider a table of sorts)
		switch (s_parameters.m_algorithm)
		{
		default:
		/*
			Single carrier & modulator
		*/
		case Algorithm::kSingle:
			{
				// Carrier
				voice.m_operators[0].enabled = true;
				voice.m_operators[0].routing = 1;
				voice.m_operators[0].isCarrier = true;
				voice.m_operators[0].oscillator.Initialize(request.form, carrierFreq, amplitude);
				voice.m_operators[0].isSlave = false;

				// Modulator #1 (sine)
				voice.m_operators[1].enabled = true;
				voice.m_operators[1].routing = 2;
				voice.m_operators[1].isCarrier = false;
				voice.m_operators[1].oscillator.Initialize(kSine, modFreq, modIndex);
				voice.m_operators[1].isSlave = false;

				// Modulator #2 (triangle, sharper)
				voice.m_operators[2].enabled = true;
				voice.m_operators[2].routing = -1;
				voice.m_operators[2].isCarrier = false;
				voice.m_operators[2].oscillator.Initialize(kPolyTriangle, modFreq, brightness);
				voice.m_operators[2].isSlave = false;
			}

		/*
			Dual carrier with slight detune
		*/
		case Algorithm::kDoubleCarriers:
			{
				// Carrier #1
				voice.m_operators[0].enabled = true;
				voice.m_operators[0].routing = 2;
				voice.m_operators[0].isCarrier = true;
				voice.m_operators[0].oscillator.Initialize(request.form, carrierFreq, amplitude);

				// Detune a few cents (sounds a bit like contained ring modulation)
				const float detune = powf(2.f, (0.04f*s_parameters.m_doubleDetune)/12.f);
				const float slaveFreq = carrierFreq*detune;
				const float slaveAmp = s_parameters.m_doubleVolume*amplitude*dBToAmplitude(-3.f);

				// Carrier #2
				voice.m_operators[1].enabled = true;
				voice.m_operators[1].routing = 2;
				voice.m_operators[1].isCarrier = true;
				voice.m_operators[1].oscillator.Initialize(request.form, slaveFreq, slaveAmp);
				voice.m_operators[1].isSlave = false;

				// Modulator #1 (sine)
				voice.m_operators[2].enabled = true;
				voice.m_operators[2].routing = 3;
				voice.m_operators[2].isCarrier = false;
				voice.m_operators[2].oscillator.Initialize(kSine, modFreq, modIndex);
				voice.m_operators[2].isSlave = false;

				// Modulator #2 (triangle, sharper)
				voice.m_operators[3].enabled = true;
				voice.m_operators[3].routing = -1;
				voice.m_operators[3].isCarrier = false;
				voice.m_operators[3].oscillator.Initialize(kPolyTriangle, modFreq, brightness);
				voice.m_operators[3].isSlave = false;
			}

			break;

		/*
			MiniMOOG model D-style 3 carriers with detune (can be synchronized)
		*/
		case Algorithm::kMiniMOOG:
			{
				// Carrier #1
				voice.m_operators[0].enabled = true;
				voice.m_operators[0].routing = 3;
				voice.m_operators[0].isCarrier = true;
				voice.m_operators[0].oscillator.Initialize(request.form, carrierFreq, amplitude*s_parameters.m_carrierVol[0]);

				// Detune a few cents (sounds a bit like contained ring modulation)
				const float amount = -1.f*kMOOGDetuneRange + 2.f*s_parameters.m_slavesDetune*kMOOGDetuneRange;
				const float detune = powf(2.f, amount/12.f);
				const float slaveFreq = carrierFreq*detune;
				const float slaveAmp = amplitude*dBToAmplitude(-3.f);

				// Carrier #2
				voice.m_operators[1].enabled = true;
				voice.m_operators[1].routing = 3;
				voice.m_operators[1].isCarrier = true;
				voice.m_operators[1].oscillator.Initialize(s_parameters.m_slaveForm1, slaveFreq, slaveAmp*s_parameters.m_carrierVol[1]);
				voice.m_operators[1].isSlave = true;
				voice.m_operators[1].filter.Reset();
				voice.m_operators[1].modAmount = s_parameters.m_slaveFM;

				// Carrier #3
				voice.m_operators[2].enabled = true;
				voice.m_operators[2].routing = 3;
				voice.m_operators[2].isCarrier = true;
				voice.m_operators[2].oscillator.Initialize(s_parameters.m_slaveForm2, slaveFreq, slaveAmp*s_parameters.m_carrierVol[2]);
				voice.m_operators[2].isSlave = true;
				voice.m_operators[2].filter.Reset();
				voice.m_operators[1].modAmount = s_parameters.m_slaveFM;

				// Modulator #1 (sine)
				voice.m_operators[3].enabled = true;
				voice.m_operators[3].routing = 4;
				voice.m_operators[3].isCarrier = false;
				voice.m_operators[3].oscillator.Initialize(kSine, modFreq, modIndex);
				voice.m_operators[3].isSlave = false;

				// Modulator #2 (triangle, sharper)
				voice.m_operators[4].enabled = true;
				voice.m_operators[4].routing = -1;
				voice.m_operators[4].isCarrier = false;
				voice.m_operators[4].oscillator.Initialize(kPolyTriangle, modFreq, brightness);
				voice.m_operators[4].isSlave = false;

				// Hard sync. slaves?
				if (true == s_parameters.m_hardSync)
				{
					voice.m_operators[1].oscillator.SyncTo(carrierFreq);
					voice.m_operators[2].oscillator.SyncTo(carrierFreq);
				}
			}

			break;
		}

		// Copy voice ADSR for now (FIXME)
		voice.m_modADSR = s_voiceADSRs[iVoice];

		// Set FM vibrato
		const float modVibratoFreq = s_parameters.m_modVibrato*goldenTen*velocity;
		voice.m_modVibrato.Initialize(kCosine, modVibratoFreq, 1.f);

		// Initialize amplitude modulator (tremolo)
		const float tremolo = s_parameters.m_tremolo;
		const float tremoloFreq = tremolo*0.25f*goldenTen * velocity;
		voice.m_AM.Initialize(kCosine, tremoloFreq, kMaxVoiceAmp);

		// One shot?
		voice.m_oneShot = s_parameters.m_loopWaves ? false : oscIsWavetable(request.form);

		// Pulse osc. duty cycle
		voice.m_pulseWidth = 0.1f + 0.8f*s_parameters.m_pulseWidth;

		// Get & reset filter
		switch (s_parameters.m_curFilter)
		{
		default:
		// Agressive and punchy filter; perfect default
		case kButterworthFilter:
			voice.m_pFilter = s_butterFilters+iVoice;
			break;

		// Ladder filter that's a bit more expressive
		case kTeemuFilter: 
			voice.m_pFilter = s_teemuFilters+iVoice;
			break;

		// Rather tame, but has the tendency to self-oscillate quickly at higher resonance
		case kImprovedMOOGFilter:  
			voice.m_pFilter = s_improvedFilters+iVoice;
			break;
		}

		voice.m_pFilter->Reset();	

		// Set ADSRs (voice & filter)
		s_voiceADSRs[iVoice].Start(s_parameters.m_voiceADSR, velocity);
		voice.m_pFilter->Start(s_sampleCount, s_parameters.m_filterADSR, velocity);

		// Reset formant shaper
		s_formantShapers[iVoice].Reset();
		
		voice.m_enabled = true;
		++s_active;

		// Store index if wanted
		if (nullptr != request.pIndex)
			*request.pIndex = iVoice;
		
	}

	// - Hxandle all requested voice releases
	// - Updates all voices as follows:
	// - Update voices (includes termination)
	// - Trigger all requested voices
	static void UpdateVoices()
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		// Process release requests
		for (auto &request : s_voiceReleaseReq)
		{
			SFM_ASSERT(-1 != request.index);

			// Stop voice ADSR
			s_voiceADSRs[request.index].Stop(request.velocity);

			Log("Voice released: " + std::to_string(request.index));
		}

		// Update active voices
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			DX_Voice &voice = s_DXvoices[iVoice];
			const bool enabled = voice.m_enabled;

			if (true == enabled)
			{
				// Set slave lowpass cutoff
				voice.SetSlaveCutoff(s_parameters.m_slavesLP);
				
				// See if any voices are done
				bool free = false;
				
				// If ADSR envelope has ran it's course, voice can be freed
				ADSR &envelope = s_voiceADSRs[iVoice];
				if (true == envelope.IsIdle())
					free = true;

				// One-shot done?
				if (true == voice.m_oneShot && true == voice.HasCycled())
					free = true;
				
				// Free
				if (true == free)
				{
					voice.m_enabled = false;
					--s_active;
				}
			}
		}

		// Spawn new voice(s)
		while (s_voiceReq.size() > 0 && s_active < kMaxVoices-1)
		{
			// Pick first free voice (FIXME: need proper heuristic in case we're out of voices)
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];
				if (false == voice.m_enabled)
				{
					const VoiceRequest request = s_voiceReq.front();
					InitializeDXVoice(request, iVoice);
					s_voiceReq.pop_front();

					Log("Voice triggered: " + std::to_string(iVoice));

					break;
				}
			}
		}

		// All release requests have been honoured; as for triggers, they may have to wait
		// if there's no slot available at this time.
		s_voiceReleaseReq.clear();
	}

	/*
		Parameter update.

		Currently there's only one of these for the Oxygen 49 + Arturia BeatStep driver.

		Ideally all inputs interact on sample level, but this is impractical and only done for a few parameters.

		FIXME:
			- Update parameters every N samples.
			- Remove all rogue parameter probes.
			  + WinMidi_GetPitchBend()
			  + WinMidi_GetMasterDrive()

		Most tweaking done here on the normalized input parameters should be adapted for the first VST attempt.
	*/

	static void UpdateState_Oxy49_BeatStep(float time)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);

		// Algorithm
		s_parameters.m_algorithm = WinMidi_GetAlgorithm();
		SFM_ASSERT(s_parameters.m_algorithm < kNumAlgorithms);
		
		// For algorithm #2
		s_parameters.m_doubleDetune = WinMidi_GetDoubleDetune();
		s_parameters.m_doubleVolume = WinMidi_GetDoubleVolume();

		// For algorithm #3
		const float slavesLP = WinMidi_GetSlavesLowpass();
		s_parameters.m_slavesDetune  = WinMidi_GetSlavesDetune();
		s_parameters.m_slavesLP      = std::max<float>(kEpsilon, clampf(0.f, 1.f, invsqrf(slavesLP)));
		s_parameters.m_hardSync      = WinMidi_GetHardSync();
		s_parameters.m_slaveFM       = WinMidi_GetSlaveFM();
		s_parameters.m_slaveForm1    = WinMidi_GetCarrierOscillator2();
		s_parameters.m_slaveForm2    = WinMidi_GetCarrierOscillator3();
		s_parameters.m_carrierVol[0] = WinMidi_GetCarrierVolume1();
		s_parameters.m_carrierVol[1] = WinMidi_GetCarrierVolume2();
		s_parameters.m_carrierVol[2] = WinMidi_GetCarrierVolume3();

		// Drive
		s_parameters.m_drive = dBToAmplitude(kDriveHidB)*WinMidi_GetMasterDrive();

		// Note (VCO) drift
		s_parameters.m_noteDrift = WinMidi_GetNoteDrift();
	
		// Modulation index
		const float alpha = 1.f/dBToAmplitude(-12.f);
		s_parameters.m_modIndex = WinMidi_GetModulationIndex()*alpha;

		// Get modulation ratio
		const unsigned ratioIdx = unsigned(WinMidi_GetModulationRatio()*(g_CM_size-1));
		s_parameters.m_modRatioM = (float) g_CM[ratioIdx][1];
		s_parameters.m_modRatioC = (float) g_CM[ratioIdx][0];

		unsigned test = g_CM_size;

		// Modulation brightness affects the modulator's oscillator blend
		s_parameters.m_modBrightness = WinMidi_GetModulationBrightness();

		// Modulation index LFO frequency
		const float frequency = WinMidi_GetModulationVibrato();
		s_parameters.m_modVibrato = frequency;

		// Noise
		s_parameters.m_noisyness = WinMidi_GetNoisyness();

		// Formant (vowel)
		s_parameters.m_formantVowel = WinMidi_GetFormantVowel();
		s_parameters.m_formant = WinMidi_GetFormant();
		s_parameters.m_formantStep =  WinMidi_GetFormantStep();

		// Nintendize
		s_parameters.m_Nintendize = WinMidi_GetNintendize();

		// Tremolo
		s_parameters.m_tremolo = WinMidi_GetTremolo();

		// Wavetable one-shot yes/no
		s_parameters.m_loopWaves = WinMidi_GetLoopWaves();

		// Pulse osc. width
		s_parameters.m_pulseWidth = WinMidi_GetPulseWidth();

		// Bass boost
		s_parameters.m_bassBoost = WinMidi_GetBassBoost();

		// Voice ADSR	
		s_parameters.m_voiceADSR.attack  = WinMidi_GetAttack();
		s_parameters.m_voiceADSR.decay   = WinMidi_GetDecay();
		s_parameters.m_voiceADSR.release = WinMidi_GetRelease();
		s_parameters.m_voiceADSR.sustain = WinMidi_GetSustain();

		// Filter ADS(R)
		s_parameters.m_filterADSR.attack  = WinMidi_GetFilterAttack();
		s_parameters.m_filterADSR.decay   = WinMidi_GetFilterDecay();
		s_parameters.m_filterADSR.sustain = WinMidi_GetFilterSustain();
		s_parameters.m_filterADSR.release = 0.f;

		// Filter parameters
		s_parameters.m_curFilter = WinMidi_GetCurFilter();
		s_parameters.m_filterContour = WinMidi_GetFilterContour();
		s_parameters.m_flipFilterEnv = WinMidi_GetFilterFlipEnvelope();
		s_parameters.m_filterParams.drive = dBToAmplitude(kMaxFilterDrivedB)*WinMidi_GetFilterDrive();
		s_parameters.m_filterParams.cutoff = WinMidi_GetFilterCutoff();
		s_parameters.m_filterParams.resonance = WinMidi_GetFilterResonance();

		// Feedback parameters
		s_parameters.m_feedback = WinMidi_GetFeedback();
		s_parameters.m_feedbackWetness = WinMidi_GetFeedbackWetness();
		s_parameters.m_feedbackPitch = -1.f + WinMidi_GetFeedbackPitch()*2.f;
	}

	/*
		Render function.
	*/

	alignas(16) static float s_voiceBuffers[kMaxVoices][kRingBufferSize];

	SFM_INLINE void ProcessDelay(float &mix)
	{
		// Process delay
		const float delayPitch = s_parameters.m_feedbackPitch;
		const float delayed = s_delayMatrix.Read(delayPitch)*kFeedbackAmplitude;
		s_delayMatrix.Write(mix, delayed*s_parameters.m_feedback);

		// Apply delay
		mix = mix + s_parameters.m_feedbackWetness*delayed;
	}

	// Shave bits off sample
	// This used to be slow (conversion) but with the right parameters the compiler doesn't make it a big deal
	SFM_INLINE float Nintendize(float sample)
	{
		int8_t quantized = int8_t(sample*127.f);
		quantized &= ~63;
		return quantized/127.f;
	}

	// Returns loudest signal (linear amplitude)
	static float Render(float time)
	{
		static float loudest = 0.f;

		// Lock ring buffer
		std::lock_guard<std::mutex> ringLock(s_ringBufMutex);

		// See if there's enough space in the ring buffer to justify rendering
		if (true == s_ringBuf.IsFull())
			return loudest;

		const unsigned available = s_ringBuf.GetAvailable();
		if (available > kMinSamplesPerUpdate)
			return loudest;

		const unsigned numSamples = kRingBufferSize-available;

		// Lock state
		std::lock_guard<std::mutex> stateLock(s_stateMutex);

		// Handle voice logic
		UpdateVoices();

		const unsigned numVoices = s_active;

		if (0 == numVoices)
		{
			loudest = 0.f;

			// Render silence (we still have to run the effects)
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				float mix = 0.f;
				ProcessDelay(mix);
				s_ringBuf.Write(mix);
			}
		}
		else
		{
			// Render dry samples for each voice (feedback)
			unsigned curVoice = 0;
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];

				if (true == voice.m_enabled)
				{
					// This should keep as close to the sample as possible (FIXME)
					const float bend = WinMidi_GetPitchBend()*kPitchBendRange;
					voice.SetPitchBend(bend);

					ADSR &voiceADSR = s_voiceADSRs[iVoice];

					FormantShaper &shaper = s_formantShapers[iVoice];

					const FormantShaper::Vowel vowel = s_parameters.m_formantVowel;
					const float formant = s_parameters.m_formant;
					const float formantStep = s_parameters.m_formantStep; // Between current and next vowel
	
					float *buffer = s_voiceBuffers[curVoice];
					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						/* const */ float sample = voice.Sample(s_parameters);

						// Blend with Nintendized version
						const float Nintendized = Nintendize(sample);
						sample = lerpf<float>(sample, Nintendized, invsqrf(s_parameters.m_Nintendize));

						// Shape by formant
						float shaped = shaper.Apply(sample, vowel, formantStep);
						sample = lerpf<float>(sample, shaped, formant);

						buffer[iSample] = sample;
					}

					// Filter voice
					voice.m_pFilter->SetLiveParameters(s_parameters.m_filterParams);
					voice.m_pFilter->Apply(buffer, numSamples, s_parameters.m_filterContour, s_parameters.m_flipFilterEnv);

					// Apply (carrier) ADSR
					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						const float ADSR = voiceADSR.Sample();
						const float sample = buffer[iSample];
						buffer[iSample] = sample*ADSR;
					}

					++curVoice; // Do *not* use to index anything other than the temporary buffers
				}
			}

			loudest = 0.f;

			// Mix & store voices
			const float bassBoost = s_parameters.m_bassBoost;

			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				const unsigned sampleCount = s_sampleCount+iSample;

				float mix = 0.f;
				for (unsigned iVoice = 0; iVoice < numVoices; ++iVoice)
				{
					const float sample = s_voiceBuffers[iVoice][iSample];
					SampleAssert(sample);

					mix = SoftClamp(mix+sample);
				}
				
				// Process delay
				ProcessDelay(mix);

				// Bass boost
				s_bassBoostLPF.SetCutoff(1.f/(kMaxSubLFO + kMaxSubLFO*bassBoost));
				mix += bassBoost*s_bassBoostLPF.Apply(mix);

				// Drive
				const float drive = WinMidi_GetMasterDrive();
				mix = ultra_tanhf(mix*drive);

				// Write
				SampleAssert(mix);
				s_ringBuf.Write(mix);

				loudest = std::max<float>(loudest, fabsf(mix));
			}
		}

		s_sampleCount += numSamples;

		return loudest;
	}

}; // namespace SFM

using namespace SFM;

/*
	SDL2 stream callback.
*/

static void SDL2_Callback(void *pData, uint8_t *pStream, int length)
{
	const unsigned numSamplesReq = length/sizeof(float);

	std::lock_guard<std::mutex> lock(s_ringBufMutex);
	{
		const unsigned numSamplesAvail = s_ringBuf.GetAvailable();
		const unsigned numSamples = std::min<unsigned>(numSamplesAvail, numSamplesReq);

		if (numSamplesAvail < numSamplesReq)
		{
//			SFM_ASSERT(false);
			Log("Buffer underrun");
		}

		float *pWrite = reinterpret_cast<float*>(pStream);
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			*pWrite++ = s_ringBuf.Read();

		s_sampleOutCount += numSamples;
	}
}

/*
	Global (de-)initialization.
*/

#include "synth-test.h"

bool Syntherklaas_Create()
{
	// Calc. LUTs
	CalculateLUTs();
	CalculateMidiToFrequencyLUT();

	// Reset sample count
	s_sampleCount = 0;
	s_sampleOutCount = 0;

	// Reset parameter state
	s_parameters.SetDefaults();

	// Reset runtime state
	for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
	{
		// FIXME: migrate
		s_DXvoices[iVoice].m_enabled = false;
		s_voiceADSRs[iVoice].Reset();

		s_improvedFilters[iVoice].Reset();
		s_butterFilters[iVoice].Reset();
		s_teemuFilters[iVoice].Reset();
	}

	// Reset voice deques
	s_voiceReq.clear();
	s_voiceReleaseReq.clear();

	// Reset boost LPF
	s_bassBoostLPF.Reset();

	// Oxygen 49 driver + SDL2
	const bool midiIn = WinMidi_Oxygen49_Start() && WinMidi_BeatStep_Start();
	const bool audioOut = SDL2_CreateAudio(SDL2_Callback);

	// Test
	OscTest();

	return true == midiIn && audioOut;
}

void Syntherklaas_Destroy()
{
	SDL2_DestroyAudio();
	WinMidi_Oxygen49_Stop();
	WinMidi_BeatStep_Stop();
}

/*
	Frame update (called by Bevacqua).
*/

float Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
//	ClickTest(time, 1.f);

	float loudest = 0.f;

	// Update state for M-AUDIO Oxygen 49 & Arturia BeatStep
	UpdateState_Oxy49_BeatStep(time);
	
	// Render
	loudest = Render(time);

	// Start audio stream on first call
	static bool first = false;
	if (false == first)
	{
		SDL2_StartAudio();
		first = true;

		Log("FM. BISON is up & running!");
	}

	return loudest;
}
