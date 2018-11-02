
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
#include "synth-state.h"
#include "synth-ringbuffer.h"
#include "synth-filter.h"
#include "synth-delayline.h"
#include "synth-math.h"
#include "synth-LUT.h"

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

 	static std::atomic<unsigned> s_sampleCount = 0;
	static std::atomic<unsigned> s_sampleOutCount = 0;

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

	// Global (parameters)
	static FM s_shadowState;
	static FM s_renderState;

	// Runtime
	static ADSR s_ADSRs[kMaxVoices];
	static ImprovedMOOGFilter s_improvedFilters[kMaxVoices];
	static CleanFilter s_cleanFilters[kMaxVoices];
	static TeemuFilter s_teemuFilters[kMaxVoices];
	static DelayMatrix s_delayMatrix(kSampleRate/8); // Div. by multiples of 4 sounds OK
	
	/*
		Voice API.
	*/

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

	static std::deque<VoiceRequest> s_voiceReq;
	static std::deque<VoiceReleaseRequest> s_voiceReleaseReq;

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

	static void InitializeVoice(const VoiceRequest &request, FM &state, unsigned iVoice)
	{
		Voice &voice = state.m_voices[iVoice];

		const float goldenTen = kGoldenRatio*10.f;

		const float frequency = request.frequency;
		const float velocity = request.velocity;
		const bool isWave = oscIsWavetable(request.form);

		// Algorithm
		voice.m_algorithm = state.m_algorithm;

		// Initialize carrier(s)
		const float amplitude = velocity*dBToAmplitude(kMaxVoicedB);
		const float modRatioC = true == isWave ? 1.f : state.m_modRatioC;  // No carrier modulation if wavetable osc.

		voice.m_carrierA.Initialize(s_sampleCount, request.form, amplitude, frequency*modRatioC);

		if (Voice::kDoubleCarriers == voice.m_algorithm)
		{
			// Initialize a detuned second carrier by going from 1 to a perfect-fifth (gives a thicker, almost phaser-like sound)
			const float detune = powf(3.f/2.f, (0.7f*state.m_algoTweak)/12.f);
			voice.m_carrierB.Initialize(s_sampleCount, request.form, amplitude*dBToAmplitude(-3.f), frequency*modRatioC*detune);
		}

		// Initialize freq. modulator & their pitched counterparts (for pitch bend)
		const float modFrequency = frequency*state.m_modRatioM;
		voice.m_modulator.Initialize(s_sampleCount, state.m_modIndex, modFrequency, 0.f, state.m_indexLFOFreq*goldenTen);

		// Initialize feedback delay line(s)
		voice.InitializeFeedback();

		// Initialize amplitude modulator (or 'tremolo')
		const float tremolo = state.m_tremolo;
		const float broFrequency = invsqrf(tremolo)*0.25f*goldenTen; // Not sure if I should mess with the "feel" of the control here (FIXME)
		voice.m_ampMod.Initialize(s_sampleCount, 1.f, broFrequency, kOscPeriod/4.f, 0.f);

		// FIXME: perhaps this should be optional?
		voice.m_oneShot = state.m_loopWaves ?  false : oscIsWavetable(request.form);

		// Get & reset filter
		switch (state.m_curFilter)
		{
		// #1
		default:
		case kTeemuFilter: // A bit more expressive (default)
			voice.m_pFilter = s_teemuFilters+iVoice;
			break;

		// #2
		case kImprovedMOOGFilter:
			voice.m_pFilter = s_improvedFilters+iVoice;
			break;
		
		// #3
		case kCleanFilter:
			voice.m_pFilter = s_cleanFilters+iVoice;
			break;
		}

		voice.m_pFilter->Reset();		

		// Set ADSRs (voice & filter)
		s_ADSRs[iVoice].Start(s_sampleCount, state.m_voiceADSR, velocity);
		voice.m_pFilter->Start(s_sampleCount, state.m_filterADSR, velocity);
		
		voice.m_enabled = true;
		++state.m_active;

		// Store index if wanted
		if (nullptr != request.pIndex)
			*request.pIndex = iVoice;
	}

	// Updates all voices as follows:
	// - Check if a currently active voice is to be terminated.
	// - Handle all requested voice releases.
	// - Trigger all requested voices.
	static void UpdateVoices(FM &state)
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		// See if any voices are done
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			Voice &voice = state.m_voices[iVoice];
			const bool enabled = voice.m_enabled;
			if (true == enabled)
			{
				bool free = false;
				
				// If ADSR envelope has ran it's course, voice can be freed.
				ADSR &envelope = s_ADSRs[iVoice];
				if (true == envelope.IsIdle(s_sampleCount))
					free = true;

				// One-shot done?				
				if (true == voice.m_oneShot && true == voice.HasCycled(s_sampleCount))
					free = true;
				
				// Free
				if (true == free)
				{
					voice.m_enabled = false;
					--state.m_active;
				}
			}
		}

		// Process release requests
		for (auto &request : s_voiceReleaseReq)
		{
			SFM_ASSERT(-1 != request.index);
			
			Voice &voice = state.m_voices[request.index];

			// Stop voice ADSR; filter ADSR just sustains, and that should be OK
			s_ADSRs[request.index].Stop(s_sampleCount, request.velocity);

			Log("Voice released: " + std::to_string(request.index));
		}

		while (s_voiceReq.size() > 0 && state.m_active < kMaxVoices-1)
		{
			// Pick first free voice (FIXME: need proper heuristic in case we're out of voices)
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				Voice &voice = state.m_voices[iVoice];
				if (false == voice.m_enabled)
				{
					const VoiceRequest request = s_voiceReq.front();
					InitializeVoice(request, state, iVoice);
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

		Ideally all inputs interact on sample level, but this is impractical and only done for the pitch bend wheel.
		Most tweaking done here on the normalized input parameters should be adapted for the first VST attempt.
	*/

	static void UpdateState_Oxy49_BeatStep(float time)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		// Algorithm
		state.m_algorithm = (Voice::Algorithm) WinMidi_GetAlgorithm();
		SFM_ASSERT(state.m_algorithm < Voice::kNumAlgorithms);
		state.m_algoTweak = WinMidi_GetAlgoTweak();

		// Drive
		state.m_drive = dBToAmplitude(kDriveHidB)*WinMidi_GetMasterDrive();

		// Modulation index
		const float alpha = 1.f/dBToAmplitude(-12.f);
		state.m_modIndex = WinMidi_GetModulationIndex()*alpha;

		// Get ratio from precalculated table (see synth-LUT.cpp)
		const unsigned tabIndex = (unsigned) (WinMidi_GetModulationRatio()*(g_CM_table_size-1));
		SFM_ASSERT(tabIndex < g_CM_table_size);
		state.m_modRatioC = (float) g_CM_table[tabIndex][0];
		state.m_modRatioM = (float) g_CM_table[tabIndex][1];

		// Modulation brightness affects the modulator's oscillator blend (sine <-> triangle)
		state.m_modBrightness = WinMidi_GetModulationBrightness();

		// Modulation index LFO frequency
		const float frequency = WinMidi_GetModulationLFOFrequency();
		state.m_indexLFOFreq = frequency;
		
		// Tremolo
		state.m_tremolo = WinMidi_GetTremolo();

		// Wavetable one-shot yes/no
		state.m_loopWaves = WinMidi_GetLoopWaves();

		// Pulse osc. width
		state.m_pulseWidth = kPulseWidths[WinMidi_GetPulseWidth()]; // FIXME: assert!

		// Voice ADSR	
		state.m_voiceADSR.attack  = WinMidi_GetAttack();
		state.m_voiceADSR.decay   = WinMidi_GetDecay();
		state.m_voiceADSR.release = WinMidi_GetRelease();
		state.m_voiceADSR.sustain = WinMidi_GetSustain();

		// Filter ADS(R)
		state.m_filterADSR.attack  = WinMidi_GetFilterAttack();
		state.m_filterADSR.decay   = WinMidi_GetFilterDecay();
		state.m_filterADSR.sustain = WinMidi_GetFilterSustain();
		state.m_filterADSR.release = 0.f;

		// Filter parameters
		state.m_curFilter = WinMidi_GetCurFilter();

		state.m_wetness = WinMidi_GetFilterWetness();
		state.m_filterParams.drive = dBToAmplitude(WinMidi_GetFilterDrive()*6.f); // FIXME: parameter domain
		state.m_filterParams.cutoff = WinMidi_GetFilterCutoff();
		state.m_filterParams.resonance = WinMidi_GetFilterResonance();

		// Feedback parameters
		state.m_feedback = WinMidi_GetFeedback();
		state.m_feedbackWetness = WinMidi_GetFeedbackWetness();
		state.m_feedbackPitch = -1.f + WinMidi_GetFeedbackPitch()*2.f; // FIXE: parameter domain
	}

	/*
		Render function.
	*/

	alignas(16) static float s_voiceBuffers[kMaxVoices][kRingBufferSize];

	const float kFeedbackAmplitude = dBToAmplitude(-3.f);

	SFM_INLINE void ProcessDelay(FM &state, float &mix)
	{
		// Process delay
		const float delayPitch = state.m_feedbackPitch;
		const float delayed = s_delayMatrix.Read(delayPitch)*kFeedbackAmplitude;
		s_delayMatrix.Write(mix, delayed*state.m_feedback);

		// Apply delay
		mix = mix + state.m_feedbackWetness*delayed;
	}

	// Returns loudest signal (linear amplitude)
	static float Render(float time)
	{
		static float loudest = 0.f;

		unsigned available = -1;

		// Lock all state & ring buffer
		std::lock_guard<std::mutex> stateLock(s_stateMutex);
		std::lock_guard<std::mutex> ringLock(s_ringBufMutex);

		// See if there's enough space in the ring buffer to justify rendering
		if (true == s_ringBuf.IsFull())
			return loudest;

		available = s_ringBuf.GetAvailable();
		if (available > kMinSamplesPerUpdate)
			return loudest;

		const unsigned numSamples = kRingBufferSize-available;

		// Update voices
		UpdateVoices(s_shadowState);

		// Make copy of state for rendering
		s_renderState = s_shadowState;
	
		FM &state = s_renderState;
		Voice *voices = state.m_voices;

		const unsigned numVoices = s_shadowState.m_active;

		if (0 == numVoices)
		{
			loudest = 0.f;

			// Render silence (we still have to run the delay filter)
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				float mix = 0.f;
				ProcessDelay(state, mix);
				s_ringBuf.Write(mix);
			}
		}
		else
		{
			// Render dry samples for each voice (feedback)
			unsigned curVoice = 0;
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				Voice &voice = voices[iVoice];
				ADSR &voiceADSR = s_ADSRs[iVoice];

				if (true == voice.m_enabled)
				{
					float *buffer = s_voiceBuffers[curVoice];

					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						const unsigned sampleCount = s_sampleCount+iSample;

						// Probed on this level for accuracy
						const float pitchBend = 2.f*(WinMidi_GetPitchBend()-0.5f);

						const float sample = voice.Sample(sampleCount, pitchBend, state.m_modBrightness, voiceADSR, state.m_pulseWidth);
						buffer[iSample] = sample;
					}

					voice.m_pFilter->SetLiveParameters(state.m_filterParams);
					voice.m_pFilter->Apply(buffer, numSamples, state.m_wetness, s_sampleCount);

					++curVoice; // Do *not* use for anything other than temporary buffers
				}
			}

			loudest = 0.f;

			// Mix & store voices
			static float prevMix = 0.f;
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
				ProcessDelay(state, mix);

				// Apply drive and soft clamp
				mix = ultra_tanhf(mix*state.m_drive);
				SampleAssert(mix);

				// Write
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

	// Reset shadow FM state
	s_shadowState.Reset(s_sampleCount);

	// Reset runtime state
	for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
	{
		s_ADSRs[iVoice].Reset();
		s_improvedFilters[iVoice].Reset();
		s_cleanFilters[iVoice].Reset();
		s_teemuFilters[iVoice].Reset();
	}

	// Reset voice deques
	s_voiceReq.clear();
	s_voiceReleaseReq.clear();

	// Oxygen 49 driver + SDL2
	const bool midiIn = WinMidi_Oxygen49_Start() && WinMidi_BeatStep_Start();
	const bool audioOut = SDL2_CreateAudio(SDL2_Callback);

	// Test
//	OscTest();

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
