
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
#include "synth-ring-buffer.h"
#include "synth-math.h"
#include "synth-LUT.h"
#include "synth-DX-voice.h"
#include "synth-delay-line.h"
#include "synth-one-pole.h"
#include "synth-vowel-filter.h"

// Driver: Win32 MIDI input (M-AUDIO Oxygen 49 & Arturia BeatStep)
#include "Win-MIDI-in-Oxygen49.h"
#include "Win-MIDI-in-BeatStep.h"

// Driver: SDL2 audio output
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
	static RingBuffer s_ringBuf;

	/*
		State.

		May only be altered after acquiring lock.
	*/

	static std::mutex s_stateMutex;

	struct VoiceRequest
	{
		unsigned *pIndex;
		Waveform form;
		unsigned key; // [0..127] MIDI
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
	static unsigned s_releasing = 0;

	// Vowel filter
	static VowelFilter s_vowelFilter;
	
	// Stereo chorus
	static DelayLine s_delayLine(kSampleRate);
	static Oscillator s_delaySweepL, s_delaySweepR, s_delaySweepMod;
	static LowpassFilter s_sweepLPF1, s_sweepLPF2;

	/*
		Voice API.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, unsigned key, float velocity)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);
		
		VoiceRequest request;
		request.pIndex = pIndex;
		request.form = form;
		request.key = key;
		request.velocity = velocity;
		s_voiceReq.push_back(request);
	}

	void ReleaseVoice(unsigned index, float velocity)
	{
		SFM_ASSERT(-1 != index);

		std::lock_guard<std::mutex> lock(s_stateMutex);

		VoiceReleaseRequest request; 
		request.index = index;
		request.velocity = velocity;

		s_voiceReleaseReq.push_back(request);
	}

	/*
		Voice logic.
	*/

	// Calculate operator frequency
	SFM_INLINE float CalcOpFreq(float fundamentalFreq, const FM_Patch::Operator &patchOp)
	{
		const unsigned coarse = patchOp.coarse;
		const float fine = patchOp.fine;

		float frequency;
		if (true == patchOp.fixed)
		{
			// 1, 10, 100, 100
			frequency = float(coarse);
		}
		else
		{
			// Start at fundamental frequency
			frequency = fundamentalFreq;

			// Detune (taken from Hexter)
			const float detune = patchOp.detune*14.f;
			frequency += (detune-7.f)/32.f;

			// Coarse adjustment
			SFM_ASSERT(coarse < 32);
			if (0 == coarse)
				frequency *= 0.5f;
			else
				frequency *= coarse;
		}

		// Fine adjustment
		frequency *= 1.f+fine;

		return frequency;
	}

	// Calculate operator depth/amplitude
	SFM_INLINE float CalcOpIndex(bool isCarrier, unsigned key, float velocity, const FM_Patch::Operator &patchOp)
	{
		float output = patchOp.output;

		// Apply level scaling (subtractive/linear)
		const unsigned breakpoint = patchOp.levelScaleBP;
		const unsigned numSemis = patchOp.levelScaleRange;
		const float levelStep = 1.f/numSemis;

		unsigned distance;
		float scale;
		if (key < breakpoint)
		{
			distance = breakpoint-key;
			scale = patchOp.levelScaleL;
		}
		else
		{
			distance = key-breakpoint;
			scale = patchOp.levelScaleR;
		}

		float delta = 1.f;
		if (distance < numSemis)
			delta = distance*levelStep;

		scale *= delta;
		output -= output*scale;

		// Return velocity scaled output
		return lerpf<float>(output, output*velocity, patchOp.velSens);
	}

	static void InitializeDXVoice(const VoiceRequest &request, unsigned iVoice)
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		DX_Voice &voice = s_DXvoices[iVoice];
		voice.ResetOperators();
//		voice.Reset();
		
		const unsigned key = request.key;
		/* const */ float fundamentalFreq = g_MIDIToFreqLUT[key];
		const float velocity = request.velocity;

		const float liveliness = s_parameters.liveliness;
		const int noteJitter = int(ceilf(oscWhiteNoise() * liveliness*kMaxNoteJitter)); // In cents
		if (0 != noteJitter)
			fundamentalFreq *= powf(2.f, (noteJitter*0.01f)/12.f);
		
		FM_Patch &patch = s_parameters.patch;

#if 1
		/*
			Test algorithm: single carrier & modulator
		*/

		// Operator #1
		voice.m_operators[0].enabled = true;
		voice.m_operators[0].modulators[0] = 1;
		voice.m_operators[0].isCarrier = true;
		voice.m_operators[0].oscillator.Initialize(
			request.form, 
			CalcOpFreq(fundamentalFreq, patch.operators[0]), 
			CalcOpIndex(true, key, velocity, patch.operators[0]));

		// Operator #2
		voice.m_operators[1].enabled = true;
		voice.m_operators[1].feedback = 1;
		voice.m_operators[1].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[1]), 
			CalcOpIndex(false, key, velocity, patch.operators[1]));

		/*
			End of Algorithm
		*/
#endif

#if 0
		/*
			Test algorithm: Volca/DX7 algorithm #5
		*/

		for (unsigned int iOp = 0; iOp < 3; ++iOp)
		{
			const unsigned carrier = iOp<<1;
			const unsigned modulator = carrier+1;

			// Carrier
			voice.m_operators[carrier].enabled = true;
			voice.m_operators[carrier].modulators[0] = modulator;
			voice.m_operators[carrier].isCarrier = true;
			voice.m_operators[carrier].oscillator.Initialize(
				request.form, 
				CalcOpFreq(fundamentalFreq, patch.operators[carrier]), 
				CalcOpIndex(true, key, velocity, patch.operators[carrier]));

			// Modulator
			voice.m_operators[modulator].enabled = true;
			voice.m_operators[modulator].oscillator.Initialize(
				kSine, 
				CalcOpFreq(fundamentalFreq, patch.operators[modulator]), 
				CalcOpIndex(false, key, velocity, patch.operators[modulator]));
		}
		 
		// Op. #6 has feedback
		voice.m_operators[5].feedback = 5;

		/*
			End of Algorithm
		*/
#endif

#if 0
		/*
			Test algorithm: Volca/DX7 algorithm #31
		*/

		for (unsigned int iOp = 0; iOp < 5; ++iOp)
		{
			const unsigned carrier = iOp;

			// Carrier
			voice.m_operators[carrier].enabled = true;
			voice.m_operators[carrier].modulators[0] = (4 == iOp) ? 5 : -1;
			voice.m_operators[carrier].isCarrier = true;
			voice.m_operators[carrier].oscillator.Initialize(
				request.form, 
				CalcOpFreq(fundamentalFreq, patch.operators[carrier]), 
				CalcOpIndex(true, key, velocity, patch.operators[carrier]));
		}

		// Operator #6
		voice.m_operators[5].enabled = true;
		voice.m_operators[5].feedback = 5;
		voice.m_operators[5].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[5]), 
			CalcOpIndex(false, key, velocity, patch.operators[5]));

		/*
			End of Algorithm
		*/
#endif

		// Key (frequency) scaling (not to be confused with Yamaha's level scaling)
		const float freqScale = fundamentalFreq/g_MIDIFreqRange;

		// Initialize LFO
		const float phaseJitter = kMaxLFOJitter*liveliness*oscWhiteNoise(); // FIXME: might be too much
		voice.m_LFO.Initialize(kDigiTriangle, 1.f, 1.f, phaseJitter);

		// Other operator settings
		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			const FM_Patch::Operator &patchOp = s_parameters.patch.operators[iOp];
			DX_Voice::Operator &voiceOp = voice.m_operators[iOp];

			// Feedback amount
			voiceOp.feedbackAmt = patchOp.feedback;

			// LFO influence
			voiceOp.ampMod = patchOp.ampMod;
			voiceOp.pitchMod = patchOp.pitchMod;
			
			// Amount of velocity
			const float opVelocity = velocity*patchOp.velSens;

			// Start envelope
			ADSR::Parameters envParams;
			envParams.attack = patchOp.attack;
			envParams.decay = patchOp.decay;
			envParams.sustain = patchOp.sustain;
			envParams.release = patchOp.release;
			envParams.attackLevel = patchOp.attackLevel;
			voiceOp.envelope.Start(envParams, opVelocity, freqScale);
		}
		
		// Enabled, up counter		
		voice.m_enabled = true;
		++s_active;

		// Store index if wanted
		if (nullptr != request.pIndex)
			*request.pIndex = iVoice;
		
	}

	SFM_INLINE void InitializeFrontVoice(unsigned iVoice)
	{
		const VoiceRequest &request = s_voiceReq.front();
		InitializeDXVoice(request, iVoice);
		s_voiceReq.pop_front();
	}

	// Update voices and handle trigger & release
	static void UpdateVoices()
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		// Process release requests
		for (auto &request : s_voiceReleaseReq)
		{
			const unsigned index = request.index;
			SFM_ASSERT(-1 != index);
			DX_Voice &voice = s_DXvoices[index];
			
			// Release voice
			const float velocity = request.velocity;
			voice.Release(velocity);
			++s_releasing;

			Log("Voice released: " + std::to_string(request.index));
		}

		// Update active voices
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			DX_Voice &voice = s_DXvoices[iVoice];

			if (true == voice.m_enabled)
			{
				// Done releasing?
				if (true == voice.IsIdle())
				{
					// Free
					voice.m_enabled = false;
					--s_active;
					--s_releasing;

					Log("Voice freed: " + std::to_string(iVoice));
				}
			}
		}

		/*
			Voice allocation

			- First try to grab an existing free voice
			- No dice? Try releasing voice(s)

			A list of releasing voices (sortable) would be very useful to make this code a bit less amateurish
		*/

		// Spawn new voice(s) if any free voices available
		while (s_voiceReq.size() > 0 && s_active < kMaxVoices)
		{
			// Pick first free voice
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];
				if (false == voice.m_enabled)
				{
					InitializeFrontVoice(iVoice);
					Log("Voice triggered: " + std::to_string(iVoice));
					break;
				}
			}
		}

		// Alternatively try to steal a quiet voice
		while (s_voiceReq.size() > 0 && s_releasing > 0)
		{
			float lowestOutput = 1.f;
			unsigned iRelease = -1;

			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];
				if (true == voice.m_enabled /* FIXME: check if it's releasing */)
				{
					// Check output level
					const float output = voice.SummedOutput();
					if (lowestOutput >= output)
					{
						// Lowest so far
						iRelease = iVoice;
						lowestOutput = output;
					}
				}
			}

			// Got one to steal?
			if (-1 != iRelease)
			{
				DX_Voice &voice = s_DXvoices[iRelease];

				// Force free
				voice.m_enabled = false;
				--s_active;
				--s_releasing;

				InitializeFrontVoice(iRelease);

				Log("Voice triggered (stolen): " + std::to_string(iRelease) + " with output " + std::to_string(lowestOutput));
			}
			else
				break; // Nothing to steal at all, so bail
		}

		// All release requests have been honoured; note triggers that can't be made are discarded
		s_voiceReleaseReq.clear();

		if (false == s_voiceReq.empty())
			Log("Voice requests ignored: " + std::to_string(s_voiceReq.size()));

		s_voiceReq.clear();
	}

	/*
		Parameter update for Oxygen 49 + BeatStep.
	*/

	static void UpdateState_Oxy49_BeatStep(float time)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);

		// Pitch bend
		s_parameters.pitchBend = powf(2.f, WinMidi_GetPitchBend()*kPitchBendRange);

		// Modulation
		s_parameters.modulation = WinMidi_GetModulation();

		// LFO speed
		s_parameters.LFOSpeed = WinMidi_GetLFOSpeed();

		// Liveliness
		s_parameters.liveliness = WinMidi_GetLiveliness();

		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			FM_Patch::Operator &patchOp = s_parameters.patch.operators[iOp];

			// Output level + Velocity sensitivity
			patchOp.output = WinMidi_GetOpOutput(iOp);
			patchOp.velSens = WinMidi_GetOpVelSens(iOp);

			// LFO influence
			patchOp.ampMod = WinMidi_GetOpAmpMod(iOp);
			patchOp.pitchMod = WinMidi_GetOpPitchMod(iOp);

			// Feedback amount
			patchOp.feedback = WinMidi_GetOpFeedback(iOp);

			// Envelope
			patchOp.attack = WinMidi_GetOpAttack(iOp);
			patchOp.decay = WinMidi_GetOpDecay(iOp);
			patchOp.sustain = WinMidi_GetOpSustain(iOp);
			patchOp.release = WinMidi_GetOpRelease(iOp)*kReleaseMul;
			patchOp.attackLevel = WinMidi_GetOpAttackLevel(iOp);

			// Level scaling
			patchOp.levelScaleBP = WinMidi_GetOpLevelScaleBP(iOp);
			patchOp.levelScaleRange = unsigned(WinMidi_GetOpLevelScaleRange(iOp)*127.f);
			patchOp.levelScaleL = WinMidi_GetOpLevelScaleL(iOp);
			patchOp.levelScaleR = WinMidi_GetOpLevelScaleR(iOp);

			// Frequency
			const bool fixed = WinMidi_GetOpFixed(iOp);
			patchOp.fixed = fixed;
			if (false == fixed)
			{
				// Ratio
				patchOp.coarse = unsigned(WinMidi_GetOpCoarse(iOp)*31.f);
				patchOp.fine   = WinMidi_GetOpFine(iOp); // 1 octave (I believe the DX7 goes *close* to 1 octave)
			}
			else
			{
				// Fixed
				// Source: http://afrittemple.com/volca/volca_programming.pdf
				const unsigned table[] = { 1, 10, 100, 1000 };
				const unsigned index = unsigned(WinMidi_GetOpCoarse(iOp)*3.f);
				patchOp.coarse = table[index];
				patchOp.fine = WinMidi_GetOpFine(iOp)*kFixedFineScale;
			}

			patchOp.detune = WinMidi_GetOpDetune(iOp);
		}
	}

	/*
		Render function.
	*/

	alignas(16) static float s_voiceBuffers[kMaxVoices][kRingBufferSize];

	// Cheap chorus attempt (WIP/FIXME)
	SFM_INLINE void ChorusToStereo(float mix) 
	{
		s_delayLine.Write(mix);

//		s_ringBuf.Write(mix);
//		s_ringBuf.Write(mix);
//		return;

		const float modulate = s_delaySweepMod.Sample(0.f);
		const float vibrato = modulate;

		const float sweepL = s_delaySweepL.Sample(vibrato);
		const float sweepR = s_delaySweepR.Sample(-vibrato);

		// Sweep around one centre point
		const float delayCtr = kSampleRate*0.02f;
		const float range = kSampleRate*0.0025f;

		// Sweep L/R taps
		const float tapL = s_delayLine.Read(delayCtr + range*s_sweepLPF1.Apply(sweepL));
		const float tapR = s_delayLine.Read(delayCtr + range*s_sweepLPF2.Apply(sweepR));
		
		// Write
		s_ringBuf.Write(tapL);
		s_ringBuf.Write(tapR);
	}



	// Returns loudest signal (linear amplitude)
	static void Render(float time)
	{
		// Lock ring buffer
		std::lock_guard<std::mutex> ringLock(s_ringBufMutex);

		// See if there's enough space in the ring buffer to justify rendering
		if (true == s_ringBuf.IsFull())
			return;

		const unsigned available = s_ringBuf.GetAvailable()>>1;
		if (available > kMinSamplesPerUpdate)
			return;

		// Amt. of samples (stereo)
		const unsigned numSamples = (kRingBufferSize>>1)-available;

		// Lock state
		std::lock_guard<std::mutex> stateLock(s_stateMutex);

		// Update voice logic
		UpdateVoices();

		// Grab LFO speed from DX7 table
		const float LFOBend = g_DX7_LFO_speed[unsigned(s_parameters.LFOSpeed*127.f)];
	
		const unsigned numVoices = s_active;

		if (0 == numVoices)
		{
			// Render silence (we still have to run the effects)
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				ChorusToStereo(0.f);
			}
		}
		else
		{
			// Render dry samples for each voice (feedback)
			unsigned curVoice = 0;
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];

				// Apply pitch bends
				voice.SetPitchBend(s_parameters.pitchBend);
				
				// Bend LFO to current speed
				voice.m_LFO.PitchBend(LFOBend);
				
				if (true == voice.m_enabled)
				{
					float *buffer = s_voiceBuffers[curVoice];
					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						float sample = voice.Sample(s_parameters);

						buffer[iSample] = sample;
					}

					++curVoice; // Do *not* use to index anything other than the temporary buffers
				}
			}

			// Mix & store voices
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				const unsigned sampleCount = s_sampleCount+iSample;

				float mix = 0.f;
				for (unsigned iVoice = 0; iVoice < numVoices; ++iVoice)
				{
					const float sample = s_voiceBuffers[iVoice][iSample];
					SampleAssert(sample);

					mix = mix+sample;
				}

				// Apply distortion
//				const float distorted = s_vowelFilter.Apply(mix, VowelFilter::kA, 0.5f);
//				mix = lerpf(mix, distorted, 0.5f);

				// Apply chorus and mix stereo output to ring buffer
				ChorusToStereo(mix);
			}
		}

		s_sampleCount += numSamples;
	}

}; // namespace SFM

using namespace SFM;

/*
	SDL2 stream callback.
*/

static void SDL2_Callback(void *pData, uint8_t *pStream, int length)
{
	const unsigned numSamplesReq = length/sizeof(float)/2;

	std::lock_guard<std::mutex> lock(s_ringBufMutex);
	{
		const unsigned numSamplesAvail = s_ringBuf.GetAvailable()>>1;
		const unsigned numSamples = std::min<unsigned>(numSamplesAvail, numSamplesReq);

		if (numSamplesAvail < numSamplesReq)
		{
			Log("Buffer underrun");
		}

		float *pWrite = reinterpret_cast<float*>(pStream);
		s_ringBuf.Flush(pWrite, numSamples<<1);

		s_sampleOutCount += numSamples;
	}
}

/*
	Global (de-)initialization.
*/

bool Syntherklaas_Create()
{
	// Calc. all tables
	CalculateLUTs();

	// Reset sample count
	s_sampleCount = 0;
	s_sampleOutCount = 0;

	// Reset parameter state
	s_parameters.SetDefaults();

	// Reset runtime state
	for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		s_DXvoices[iVoice].Reset();

	// Sweep oscillators (the few arbitrary values make little sense to move to synth-global.h, IMO)
	s_delaySweepL.Initialize(kDigiTriangle, 0.5f, 0.5f, 0.f);
	s_delaySweepR.Initialize(kDigiTriangle, 0.5f, 0.5f, 0.5f);
	s_delaySweepMod.Initialize(kCosine, 0.05f, 1.f, 0.4321f);

	// Sweep LPFs (FIXME: tweak value)
	const float sweepCut = 0.005f;
	s_sweepLPF1.SetCutoff(sweepCut);
	s_sweepLPF2.SetCutoff(sweepCut); 
	
	// Reset voice deques
	s_voiceReq.clear();
	s_voiceReleaseReq.clear();

	// Oxygen 49 driver + SDL2
	const bool midiIn = WinMidi_Oxygen49_Start() && WinMidi_BeatStep_Start();
	const bool audioOut = SDL2_CreateAudio(SDL2_Callback);

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

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	// Update state for M-AUDIO Oxygen 49 & Arturia BeatStep
	UpdateState_Oxy49_BeatStep(time);
	
	// Render
	Render(time);

	// Start audio stream on first call
	static bool first = false;
	if (false == first)
	{
		SDL2_StartAudio();
		first = true;

		Log("FM. BISON 0.3 is up & running!");
	}
}
