
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
#include "synth-voice.h"
#include "synth-delay-line.h"
#include "synth-one-pole.h"

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
		unsigned key; // [0..127] (MIDI)
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
	static Voice s_voices[kMaxVoices];
	static unsigned s_active = 0;

	// Parameter filters
	static LowpassFilter s_cutoffLPF, s_resoLPF;
	
	// Chorus-to-stereo effect
	static DelayLine s_delayLine(kSampleRate);
	static Oscillator s_delaySweepL, s_delaySweepR;
	static Oscillator s_delaySweepMod;
	static LowpassFilter s_sweepLPF1, s_sweepLPF2;

	// Running LFO (used for no key sync.)
	static Oscillator s_globalLFO;

	/*
		Voice API.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, unsigned key, float velocity)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);

		SFM_ASSERT(key <= 127);
		SFM_ASSERT(velocity >= 0.f && velocity <= 1.f);

		VoiceRequest request;
		request.pIndex = pIndex;
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
			SFM_ASSERT(coarse <= 32);
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

		// Apply level scaling (subtractive & linear)
		const unsigned breakpoint = patchOp.levelScaleBP;
		const unsigned numSemis = patchOp.levelScaleRange;
		if (0 != numSemis)
		{
			const float levelStep = 1.f/numSemis;

			unsigned distance;
			float amount;
			if (key < breakpoint)
			{
				distance = breakpoint-key;
				amount = patchOp.levelScaleL;
			}
			else
			{
				distance = key-breakpoint;
				amount = patchOp.levelScaleR;
			}

			// Apply linear scale over selected range
			distance = std::min<unsigned>(numSemis, distance);
			const float scale = 1.f-(distance*levelStep);
			
			// Apply as needed
			output = lerpf<float>(output, output*scale, amount);

			SFM_ASSERT(output >= 0.f && output <= 1.f);
		}

		if (true == isCarrier)
			// Scale to max. amplitude
			output *= kMaxVoiceAmp;

		// Return velocity scaled output
		return lerpf<float>(output, output*velocity, patchOp.velSens);
	}

	// Calculate phase jitter
	SFM_INLINE float CalcPhaseJitter(float liveliness)
	{
		const float random = mt_randf();
		return random*0.25f*liveliness; // 90 deg. max.
	}

	static void InitializeVoice(const VoiceRequest &request, unsigned iVoice)
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		Voice &voice = s_voices[iVoice];
		voice.Reset();

		const unsigned key = request.key;
		/* const */ float fundamentalFreq = g_MIDIToFreqLUT[key];  
		const float velocity = powf(request.velocity, 3.f); // Raise velocity (source: Jan Marguc)
		
		const float liveliness = s_parameters.liveliness;
		const int noteJitter = int(ceilf(oscWhiteNoise() * liveliness*kMaxNoteJitter)); // In cents
		if (0 != noteJitter)
			fundamentalFreq *= powf(2.f, (noteJitter*0.01f)/12.f);
		
		FM_Patch &patch = s_parameters.patch;
	
#if 0
		/*
			Test algorithm: single carrier & modulator
		*/

		// Operator #1
		voice.m_operators[0].enabled = true;
		voice.m_operators[0].modulators[0] = 1;
		voice.m_operators[0].feedback = 0;
		voice.m_operators[0].isCarrier = true;
		voice.m_operators[0].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[0]), 
			CalcOpIndex(true, key, velocity, patch.operators[0]),
			CalcPhaseJitter(liveliness));

		// Operator #2
		voice.m_operators[1].enabled = true;
		voice.m_operators[1].feedback = 1;
		voice.m_operators[1].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[1]), 
			CalcOpIndex(false, key, velocity, patch.operators[1]),
			CalcPhaseJitter(liveliness));

		/*
			End of Algorithm
		*/
#endif

#if 1
		/*
			Test algorithm: Electric piano (Wurlitzer style)
		*/

		// Select Wurlitzer mode
		voice.m_mode = Voice::kWurlitzer;

		// Reset Wurlitzer grit filter
		voice.m_linVel = request.velocity;
		voice.m_grit.Reset(velocity /* Use as grit filter cutoff (FIXME) */);
		
		// Carrier (pure)
		voice.m_operators[0].enabled = true;
		voice.m_operators[0].feedback = 0;
 		voice.m_operators[0].modulators[0] = 1;
		voice.m_operators[0].modulators[1] = 3;
		voice.m_operators[0].modulators[2] = 5;
		voice.m_operators[0].isCarrier = true;
		voice.m_operators[0].oscillator.Initialize(
			kSine,
			0.f,
			CalcOpIndex(true, key, velocity, patch.operators[0]));

		// C <- 2 <- 3
		voice.m_operators[1].enabled = true;
		voice.m_operators[1].modulators[0] = 2;
		voice.m_operators[1].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[1]), 
			CalcOpIndex(false, key, velocity, patch.operators[1]),
			CalcPhaseJitter(liveliness));
		
		voice.m_operators[2].enabled = true;
		voice.m_operators[2].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[2]), 
			CalcOpIndex(false, key, velocity, patch.operators[2]),
			CalcPhaseJitter(liveliness));

		// C <- 4 <- 5
		voice.m_operators[3].enabled = true;
		voice.m_operators[3].modulators[0] = 4;
		voice.m_operators[3].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[3]), 
			CalcOpIndex(false, key, velocity, patch.operators[3]),
			CalcPhaseJitter(liveliness));

		voice.m_operators[4].enabled = true;
		voice.m_operators[4].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[4]), 
			CalcOpIndex(false, key, velocity, patch.operators[4]),
			CalcPhaseJitter(liveliness));

		// C <- 6
		voice.m_operators[5].enabled = true;
		voice.m_operators[5].oscillator.Initialize(
			kSine,
			CalcOpFreq(fundamentalFreq, patch.operators[5]), 
			CalcOpIndex(false, key, velocity, patch.operators[5]),
			CalcPhaseJitter(liveliness));

		/*
			End of Algorithm
		*/
#endif

#if 0
		/*
			Test algorithm: Volca/DX7 algorithm #2
		*/

		// Operator #1
		voice.m_operators[0].enabled = true;
		voice.m_operators[0].modulators[0] = 1;
		voice.m_operators[0].isCarrier = true;
		voice.m_operators[0].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[0]), 
			CalcOpIndex(true, key, velocity, patch.operators[0]),
			CalcPhaseJitter(liveliness));

		// Operator #2
		voice.m_operators[1].enabled = true;
		voice.m_operators[1].feedback = 1;
		voice.m_operators[1].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[1]), 
			CalcOpIndex(false, key, velocity, patch.operators[1]),
			CalcPhaseJitter(liveliness));

		// Operator #3
		voice.m_operators[2].enabled = true;
		voice.m_operators[2].modulators[0] = 3;
		voice.m_operators[2].isCarrier = true;
		voice.m_operators[2].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[2]), 
			CalcOpIndex(true, key, velocity, patch.operators[2]),
			CalcPhaseJitter(liveliness));

		// Operator #4
		voice.m_operators[3].enabled = true;
		voice.m_operators[3].modulators[0] = 4;
		voice.m_operators[3].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[3]), 
			CalcOpIndex(false, key, velocity, patch.operators[3]),
			CalcPhaseJitter(liveliness));

		// Operator #5
		voice.m_operators[4].enabled = true;
		voice.m_operators[4].modulators[0] = 5;
		voice.m_operators[4].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[4]), 
			CalcOpIndex(false, key, velocity, patch.operators[4]),
			CalcPhaseJitter(liveliness));

		// Operator #6
		voice.m_operators[5].enabled = true;
		voice.m_operators[5].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[5]), 
			CalcOpIndex(false, key, velocity, patch.operators[5]),
			CalcPhaseJitter(liveliness));

		/*
			End of Algorithm
		*/
#endif

#if 0
		/*
			Test algorithm: Volca/DX7 algorithm #5 (used for E. Piano)
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
				kSine, 
				CalcOpFreq(fundamentalFreq, patch.operators[carrier]), 
				CalcOpIndex(true, key, velocity, patch.operators[carrier]),
				CalcPhaseJitter(liveliness));

			// Modulator
			voice.m_operators[modulator].enabled = true;
			voice.m_operators[modulator].oscillator.Initialize(
				kSine, 
				CalcOpFreq(fundamentalFreq, patch.operators[modulator]), 
				CalcOpIndex(false, key, velocity, patch.operators[modulator]),
				CalcPhaseJitter(liveliness));
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
				kSine, 
				CalcOpFreq(fundamentalFreq, patch.operators[carrier]), 
				CalcOpIndex(true, key, velocity, patch.operators[carrier]),
				CalcPhaseJitter(liveliness));
		}

		// Operator #6
		voice.m_operators[5].enabled = true;
		voice.m_operators[5].feedback = 5;
		voice.m_operators[5].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[5]), 
			CalcOpIndex(false, key, velocity, patch.operators[5]),
			CalcPhaseJitter(liveliness));

		/*
			End of Algorithm
		*/
#endif

#if 0
		/*
			Test algorithm: Volca/DX7 algorithm #32
		*/

		for (unsigned int iOp = 0; iOp < 6; ++iOp)
		{
			const unsigned carrier = iOp;

			// Carrier
			voice.m_operators[carrier].enabled = true;
			voice.m_operators[carrier].isCarrier = true;
			voice.m_operators[carrier].oscillator.Initialize(
				kSine, 
				CalcOpFreq(fundamentalFreq, patch.operators[carrier]), 
				CalcOpIndex(true, key, velocity, patch.operators[carrier]),
				CalcPhaseJitter(liveliness));
		}
		
		// Operator #6 has feedback on itself
		voice.m_operators[5].feedback = 5;

		/*
			End of Algorithm
		*/
#endif

#if	0
		/*
			Test algorithm: feedback festival (use to create a super saw)
		*/

		for (unsigned int iOp = 0; iOp < 6; ++iOp)
		{
			const unsigned carrier = iOp;

			// Carrier
			voice.m_operators[carrier].enabled = true;
			voice.m_operators[carrier].feedback = carrier;
			voice.m_operators[carrier].isCarrier = true;
			voice.m_operators[carrier].oscillator.Initialize(
				kSine, 
				CalcOpFreq(fundamentalFreq, patch.operators[carrier]), 
				CalcOpIndex(true, key, velocity, patch.operators[carrier]),
				CalcPhaseJitter(liveliness));
		}
		
		// Odd operators modulate even ones
		voice.m_operators[0].modulators[0] = 1;
		voice.m_operators[2].modulators[0] = 3;
		voice.m_operators[4].modulators[0] = 5;

		/*
			End of Algorithm
		*/
#endif

#if 0
		/*
			Volca/DX algorithm #17
		*/

		// Operator #1
		voice.m_operators[0].enabled = true;
		voice.m_operators[0].modulators[0] = 1;
		voice.m_operators[0].modulators[1] = 2;
		voice.m_operators[0].modulators[2] = 4;
		voice.m_operators[0].isCarrier = true;
		voice.m_operators[0].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[0]), 
			CalcOpIndex(true, key, velocity, patch.operators[0]),
			CalcPhaseJitter(liveliness));

		// Operator C <- #2
		voice.m_operators[1].enabled = true;
		voice.m_operators[1].feedback = 1;
		voice.m_operators[1].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[1]), 
			CalcOpIndex(false, key, velocity, patch.operators[1]),
			CalcPhaseJitter(liveliness));

		// Operator C <- #3 <- #4
		voice.m_operators[2].enabled = true;
		voice.m_operators[2].modulators[0] = 3;
		voice.m_operators[2].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[2]), 
			CalcOpIndex(false, key, velocity, patch.operators[2]),
			CalcPhaseJitter(liveliness));

		voice.m_operators[3].enabled = true;
		voice.m_operators[3].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[3]), 
			CalcOpIndex(false, key, velocity, patch.operators[3]),
			CalcPhaseJitter(liveliness));

		// Operator C <- #5 <- #6
		voice.m_operators[4].enabled = true;
		voice.m_operators[4].modulators[0] = 5;
		voice.m_operators[4].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[4]), 
			CalcOpIndex(false, key, velocity, patch.operators[4]),
			CalcPhaseJitter(liveliness));

		voice.m_operators[5].enabled = true;
		voice.m_operators[5].oscillator.Initialize(
			kSine, 
			CalcOpFreq(fundamentalFreq, patch.operators[5]), 
			CalcOpIndex(false, key, velocity, patch.operators[5]),
			CalcPhaseJitter(liveliness));
#endif

		// Initialize LFO
		const float phaseAdj = (true == s_parameters.LFOSync) ? 0.f : s_globalLFO.GetPhase();
		const float phaseJitter = CalcPhaseJitter(liveliness);
		voice.m_LFO.Initialize(kDigiTriangle, s_globalLFO.GetFrequency(), 1.f, phaseAdj+phaseJitter);

		// Other operator settings
		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			const FM_Patch::Operator &patchOp = s_parameters.patch.operators[iOp];
			Voice::Operator &voiceOp = voice.m_operators[iOp];

			// Feedback amount
			voiceOp.feedbackAmt = patchOp.feedback;

			// LFO influence
			voiceOp.ampMod = patchOp.ampMod;
			voiceOp.pitchMod = patchOp.pitchMod;

			// Distortion
			voiceOp.distortion = patchOp.distortion;
			
			// Amount of velocity
			const float opVelocity = velocity*patchOp.velSens;

			// Set up & start envelope
			ADSR::Parameters envParams;
			envParams.attack = patchOp.attack;
			envParams.decay = patchOp.decay;
			envParams.sustain = patchOp.sustain;
			envParams.release = patchOp.release;
			envParams.attackLevel = patchOp.attackLevel;
			
			// The multiplier is between 0.1 and 10.0 (nicked from Arturia DX7-V's manual) and rate scaling can *double*
			// that value at the highest available frequency (note) when fully applied
			// This is not exactly like a DX7 does it, but I am not interested in emulating all legacy logic!
			const float rateScale = kEnvMulMin + kEnvMulRange*patchOp.envRateMul;
			const float envScale = rateScale + rateScale*patchOp.envRateScale*(request.key/127.f); // FIXME: offer a range!

			voiceOp.envelope.Start(envParams, opVelocity, envScale);
		}

		// Reset filter
		voice.m_LPF.resetState();

		// Set pitch envelope
		voice.m_pitchEnvInvert = patch.pitchEnvInvert;
		voice.m_pitchEnvBias = patch.pitchEnvBias;

		ADSR::Parameters envParams;
		envParams.attack  = patch.pitchEnvAttack;
		envParams.decay   = patch.pitchEnvDecay;
		envParams.sustain = patch.pitchEnvSustain;
		envParams.release = patch.pitchEnvRelease;
		envParams.attackLevel = patch.pitchEnvLevel;
		voice.m_pitchEnv.Start(envParams, 0.f, 1.f); // No velocity response (FIXME: scale attack level?)
		
		// Enabled, up counter		
		voice.m_state = Voice::kEnabled;
		++s_active;

		// Store index if wanted
		if (nullptr != request.pIndex)
			*request.pIndex = iVoice;
		
	}

	SFM_INLINE void InitializeFrontmostVoice(unsigned iVoice)
	{
		const VoiceRequest &request = s_voiceReq.front();
		InitializeVoice(request, iVoice);

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
			Voice &voice = s_voices[index];
			
			// Release voice
			const float velocity = request.velocity;
			voice.Release(velocity);

			Log("Voice released: " + std::to_string(request.index));
		}

		// Update active voices
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			Voice &voice = s_voices[iVoice];

			if (true == voice.IsActive())
			{
				if (true == voice.IsDone())
				{
					// Free
					voice.m_state = Voice::kIdle;
					--s_active;

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
				Voice &voice = s_voices[iVoice];
				if (Voice::kIdle == voice.m_state)
				{
					InitializeFrontmostVoice(iVoice);
					Log("Voice triggered: " + std::to_string(iVoice));
					break;
				}
			}
		}

		// Alternatively try to steal a quiet voice
		while (s_voiceReq.size() > 0)
		{
			float lowestOutput = 1.f;
			unsigned iRelease = -1;

			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				Voice &voice = s_voices[iVoice];
				if (Voice::kReleasing == voice.m_state)
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
				Voice &voice = s_voices[iRelease];

				// Force free
				voice.m_state = Voice::kIdle;
				--s_active;

				InitializeFrontmostVoice(iRelease);

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
		s_parameters.pitchBend = powf(2.f, WinMidi_GetPitchBend()*(kPitchBendRange/12.f));

		// Modulation
		s_parameters.modulation = WinMidi_GetModulation();

		// LFO speed
		s_parameters.LFOSpeed = WinMidi_GetLFOSpeed();

		// Liveliness
		s_parameters.liveliness = WinMidi_GetLiveliness();

		// LFO key sync.
		s_parameters.LFOSync = WinMidi_GetLFOSync();

		// Chorus
		s_parameters.chorus = WinMidi_ChorusEnabled();

		// Filter parameters
		s_parameters.cutoff = WinMidi_GetFilterCutoff();
		s_parameters.resonance = WinMidi_GetFilterResonance();

		// Grit parameter(s)
		s_parameters.gritDrive = WinMidi_GetGritDrive();
		s_parameters.gritWet = WinMidi_GetGritWet();

		// Pitch envelope
		s_parameters.patch.pitchEnvAttack = WinMidi_GetPitchEnvAttack();
		s_parameters.patch.pitchEnvDecay = WinMidi_GetPitchEnvDecay();
		s_parameters.patch.pitchEnvSustain = WinMidi_GetPitchEnvSustain();
		s_parameters.patch.pitchEnvRelease = WinMidi_GetPitchEnvRelease();
		s_parameters.patch.pitchEnvLevel = WinMidi_GetPitchEnvLevel();
		s_parameters.patch.pitchEnvBias = WinMidi_GetPitchEnvBias();
		s_parameters.patch.pitchEnvInvert = WinMidi_GetPitchEnvPolarity();

		// Set operators
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
			patchOp.levelScaleRange = unsigned(WinMidi_GetOpLevelScaleRange(iOp)*127.f); // FIXME: range?
			patchOp.levelScaleL = WinMidi_GetOpLevelScaleL(iOp);
			patchOp.levelScaleR = WinMidi_GetOpLevelScaleR(iOp);

			// Envelope rate
			const float envRateMul = WinMidi_GetOpEnvRateMul(iOp);
			patchOp.envRateMul = envRateMul;
			patchOp.envRateScale = WinMidi_GetOpEnvRateScale(iOp);

			// Distortion
			patchOp.distortion = WinMidi_GetOpDistortion(iOp);

			// Frequency
			const bool fixed = WinMidi_GetOpFixed(iOp);
			patchOp.fixed = fixed;
			if (false == fixed)
			{
				// Ratio
				patchOp.coarse = unsigned(WinMidi_GetOpCoarse(iOp)*32.f);
				patchOp.fine   = WinMidi_GetOpFine(iOp); // 1 octave (the DX7 goes *close* to 1 octave, 1.99)
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

	// Poor man's chorus intended to create a wider (stereo) mix
	SFM_INLINE void ChorusToStereo(float mix) 
	{
		s_delayLine.Write(mix);

		// Vibrate the sweep LFOs
		const float modulate = s_delaySweepMod.Sample(0.f);
		const float vibrato = modulate;
		
		// Sample sweep LFOs
		const float sweepL = s_delaySweepL.Sample(vibrato);
		const float sweepR = s_delaySweepR.Sample(-vibrato);

		// Sweep around one centre point
		const float delayCtr = kSampleRate*0.03f;
		const float range = kSampleRate*0.0025f;

		// Take sweeped L/R taps (lowpassed to circumvent artifacts)
		const float tapL = s_delayLine.Read(delayCtr + range*s_sweepLPF1.Apply(sweepL));
		const float tapR = s_delayLine.Read(delayCtr + range*s_sweepLPF2.Apply(sweepR));
		const float tapM = mix;
		
		// Write
		s_ringBuf.Write(tapM + (tapM-tapL));
		s_ringBuf.Write(tapM + (tapM-tapR));
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
		const float freqLFO = g_DX7_LFO_speed[unsigned(s_parameters.LFOSpeed*127.f)];
		s_globalLFO.SetPitch(freqLFO);

		// Update filter settings
		const float lowestC = 16.35f;
		const float cutoff = lowestC + s_cutoffLPF.Apply(s_parameters.cutoff)*(kNyquist-lowestC); // [16.0..kNyquist]
		const float Q = 0.025f + s_resoLPF.Apply(s_parameters.resonance)*kFilterMaxResonance;     // [0.025..40.0]
	
		const unsigned numVoices = s_active;

		if (0 == numVoices)
		{
			// Render silence
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				float mix = 0.f;
				
				// Apply chorus
				if (true == s_parameters.chorus)
					ChorusToStereo(mix);
				else
				{
					s_ringBuf.Write(mix);
					s_ringBuf.Write(mix);
				}
			}
		}
		else
		{
			// Render dry samples for each voice (feedback)
			unsigned curVoice = 0;
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				Voice &voice = s_voices[iVoice];

				// Update filter coefficients
				voice.m_LPF.updateCoefficients(cutoff, Q, SvfLinearTrapOptimised2::LOW_PASS_FILTER, kSampleRate);
				
				// Set LFO to current speed
				voice.m_LFO.SetPitch(freqLFO);
				
				if (true == voice.IsActive())
				{
					float *buffer = s_voiceBuffers[curVoice];
					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						const float sample = voice.Sample(s_parameters);
						buffer[iSample] = sample;
					}

					++curVoice; // Do *not* use to index anything other than the temporary buffers
				}
			}

			// Mix & store voices
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				const unsigned sampleCount = s_sampleCount+iSample;

				// Tick global LFO so it advances along with each sample rendered
				// This isn't too pretty but I think I'll solve it by providing a simplified LFO object instead of an Oscillator (FIXME)
				s_globalLFO.Sample(0.f);

				float mix = 0.f;
				for (unsigned iVoice = 0; iVoice < numVoices; ++iVoice)
				{
					const float sample = s_voiceBuffers[iVoice][iSample];
					
					// FIXME: filter goes out of bounds (see synth-voice.cpp)
//					SampleAssert(sample);

					mix = mix+sample;
				}

				// Stereo output to ring buffer
				if (true == s_parameters.chorus)
					ChorusToStereo(mix);
				else
				{
					s_ringBuf.Write(mix);
					s_ringBuf.Write(mix);
				}
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
		s_voices[iVoice].Reset();

	// Initialize main filter & it's control filters
	s_cutoffLPF.SetCutoff(kControlCutoff);
	s_resoLPF.SetCutoff(kControlCutoff);

	// Delay: sweep oscillators (the few arbitrary values make little sense to move to synth-global.h, IMO)
	s_delaySweepL.Initialize(kSine, 0.5f*kChorusRate, 0.5f, 0.f);
	s_delaySweepR.Initialize(kSine, 0.5f*kChorusRate, 0.5f, 0.33f);
	s_delaySweepMod.Initialize(kDigiTriangle, 0.05f*kChorusRate, 1.f, 0.4321f);

	// Delay: sweep LPFs
	s_sweepLPF1.SetCutoff(kControlCutoffS);
	s_sweepLPF2.SetCutoff(kControlCutoffS); 

	// Start global LFO
	s_globalLFO.Initialize(kDigiTriangle, g_DX7_LFO_speed[0], 1.f);
	
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
