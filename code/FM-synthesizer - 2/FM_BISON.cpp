
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
#include "synth-math.h"
#include "synth-LUT.h"
#include "synth-DX-voice.h"
#include "synth-filter.h"
#include "synth-delay-line.h"
#include "synth-vowel-filter.h"
#include "synth-FM-algorithms.h"

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

	// Master filters
	static KrajeskiFilter s_cleanFilters[kMaxVoices];
	static TeemuFilter s_MOOGFilters[kMaxVoices];

	// Vowel (formant) filters
	static VowelFilter s_vowelFilters[kMaxVoices];

	// Delay + LFO(s)
	static DelayLine s_delayLine;
	static Oscillator s_delayLFO_L;
	static Oscillator s_delayLFO_R;
	static Oscillator s_delayLFO_M;
	
	/*
		Voice API.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, unsigned key, float velocity)
	{
		SFM_ASSERT(key < 127);

		const float frequency = g_midiToFreqLUT[key];

		// I mean, who the f*ck decides this is necessary in an FM setting?
//		SFM_ASSERT(true == InAudibleSpectrum(frequency));

		std::lock_guard<std::mutex> lock(s_stateMutex);
		
		VoiceRequest request;
		request.pIndex = pIndex;
		request.form = form;
		request.key = key;
		request.velocity = velocity;
		s_voiceReq.push_back(request);

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

		s_voiceReleaseReq.push_back(request);
	}

	/*
		Voice logic.
	*/

	// Calculate operator frequency (taken from Volca FM guide & Hexter)
	SFM_INLINE float CalcOpFreq(float frequency, const FM_Patch::Operator &patchOp)
	{
		const unsigned coarse = patchOp.coarse;
		const float fine = patchOp.fine;

		if (true == patchOp.fixed)
		{
			// Fixed ratio (taken from Volca FM third-party manual)
			// Detune has no effect on fixed ratio?
			// The Volca seems to disagree (FIXME!)
			const float coarseTab[4] = { 1, 10, 100, 1000 };
			frequency = coarseTab[coarse&3];
		}
		else
		{
			// Not sure if this is right, took it from Hexter
			const float detune = patchOp.detune*14.f;
			frequency += (detune-7.f)/32.f;

			SFM_ASSERT(coarse < 32);
			if (0 == coarse)
				frequency *= 0.5f;
			else
				frequency *= coarse;
		}

		frequency *= 1.f+fine;

		return frequency;
	}

	SFM_INLINE float LevelScale(float scale, unsigned distance)
	{
		// FIXME
		return 0.f;
	}

	// Calculate operator amplitude/depth
	SFM_INLINE float CalculateOperatorAmplitude(bool isCarrier, unsigned key, float velocity, const FM_Patch::Operator &patchOp)
	{
		float amplitude = (true == isCarrier) ? kMaxVoiceAmp*patchOp.amplitude : patchOp.amplitude;

		// Level scaling, DX7-style (but a bit more straightforward, no per-3-note grouping et cetera)
		// FIXME: see FM_BISON.h

		const unsigned breakpoint = patchOp.levelScaleBP;
		const unsigned numSemis = 2*12; // 2 octaves (FIXME)
		const float step = 1.f/numSemis;

		// FIXME: wrap into function (above)
		if (key < breakpoint)
		{
			const unsigned distance = breakpoint-key;
			float levelScale = patchOp.levelScaleLeft;

			float delta = 1.f;
			if (distance < numSemis)
				delta = distance*step;

			levelScale *= (true == patchOp.levelScaleExpL) ? delta*delta : delta;
			amplitude += amplitude*levelScale;
//			amplitude = saturatef(amplitude);
		}
		else if (key > breakpoint)
		{
			const unsigned distance = key-breakpoint;
			float levelScale = patchOp.levelScaleRight;

			float delta = 1.f;
			if (distance < numSemis)
				delta = distance*step;

			levelScale *= (true == patchOp.levelScaleExpR) ? delta*delta : delta;
			amplitude += amplitude*levelScale;
//			amplitude = saturatef(amplitude);
		}

		if (false == isCarrier)
		{
			// Apply Hexter's DX-style EG-to-modulation table
			const int tabIndex = 128 + int(amplitude*99.f); // Yamaha EG range
			const float modIndex = g_dx7_voice_eg_ol_to_mod_index_table[tabIndex];
			amplitude = modIndex;
		}

		return lerpf<float>(amplitude, amplitude*velocity, patchOp.velSens);
	}

	SFM_INLINE float CalcCarAmp(unsigned key, float velocity, const FM_Patch::Operator &patchOp) { return CalculateOperatorAmplitude(true, key, velocity, patchOp); }
	SFM_INLINE float CalcModAmp(unsigned key, float velocity, const FM_Patch::Operator &patchOp) { return CalculateOperatorAmplitude(false, key, velocity, patchOp); }

	static void InitializeDXVoice(const VoiceRequest &request, unsigned iVoice)
	{
		SFM_ASSERT(false == s_stateMutex.try_lock());

		DX_Voice &voice = s_DXvoices[iVoice];
		voice.ResetOperators();
//		voice.Reset();
		
		const unsigned key = request.key;
		float frequency = g_midiToFreqLUT[key];

		// Randomize note frequency little if wanted (in (almost) entire cents)
		const float jitterAmt = kMaxNoteJitter*s_parameters.noteJitter;
		const int cents = int(ceilf(oscWhiteNoise()*jitterAmt));

		float jitter = 1.f;
		if (0 != cents)
			jitter = powf(2.f, (cents*0.01f)/12.f);
		
		FloatAssert(jitter);
		frequency *= jitter;

		const float velocity    = request.velocity;
		const float invVelocity = 1.f-velocity;

		// Save copy
		voice.m_velocity = velocity;

		FM_Patch &patch = s_parameters.patch;

#if 0

		/*
			Test algorithm: single carrier & modulator
		*/

		// Operator #1
		voice.m_operators[0].enabled = true;
		voice.m_operators[0].modulators[0] = 1;
		voice.m_operators[0].isCarrier = true;
		voice.m_operators[0].oscillator.Initialize(
			request.form, 
			CalcOpFreq(frequency, patch.operators[0]), 
			CalcCarAmp(key, velocity, patch.operators[0]));

		// Operator #2
		voice.m_operators[1].enabled = true;
		voice.m_operators[1].feedback = 1;
		voice.m_operators[1].oscillator.Initialize(
			kSine, 
			CalcOpFreq(frequency, patch.operators[1]), 
			CalcModAmp(key, velocity, patch.operators[1]));

		/*
			End of Algorithm
		*/

#endif

#if 0

		/*
			Test algorithm: Volca algorithm #5
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
				CalcOpFreq(frequency, patch.operators[carrier]), 
				CalcCarAmp(key, velocity, patch.operators[carrier]));

			// Modulator
			voice.m_operators[modulator].enabled = true;
			voice.m_operators[modulator].oscillator.Initialize(
				kSine, 
				CalcOpFreq(frequency, patch.operators[modulator]), 
				CalcModAmp(key, velocity, patch.operators[modulator]));
		}
		 
		// Op. #6 has feedback
		voice.m_operators[5].feedback = 5;

		/*
			End of Algorithm
		*/

#endif

#if 1

		/*
			Test algorithm: Volca algorithm #31
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
				CalcOpFreq(frequency, patch.operators[carrier]), 
				CalcCarAmp(key, velocity, patch.operators[carrier]));
		}

		// Operator #6
		voice.m_operators[5].enabled = true;
		voice.m_operators[5].feedback = 5;
		voice.m_operators[5].oscillator.Initialize(
			kSine, 
			CalcOpFreq(frequency, patch.operators[5]), 
			CalcModAmp(key, velocity, patch.operators[5]));

		/*
			End of Algorithm
		*/

#endif

		// Freq. scale (or key scale if you will, not to be confused with Yamaha's level scaling)
		const float freqScale = frequency/g_midiFreqRange;

		// Set tremolo LFO
		const unsigned tremIdx = unsigned(127.f*s_parameters.tremolo);
		const float tremFreq = g_dx7_voice_lfo_frequency[tremIdx];
		const float tremShift = s_parameters.noteJitter*kMaxTremoloJitter*oscWhiteNoise();
		voice.m_tremolo.Initialize(s_parameters.LFOform, tremFreq, 1.f, tremShift);

		// Set vibrato LFO
		const unsigned vibIdx = 4 + unsigned(freqScale*8.f);
		const float vibFreq = g_dx7_voice_lfo_frequency[vibIdx];
		const float vibShift = s_parameters.noteJitter*kMaxVibratoJitter*oscWhiteNoise();
		voice.m_vibrato.Initialize(s_parameters.LFOform, vibFreq, kMaxVibratoDepth, vibShift);

		// Set per operator
		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			const FM_Patch::Operator &patchOp = s_parameters.patch.operators[iOp];
			DX_Voice::Operator &voiceOp = voice.m_operators[iOp];

			// Adj. velocity
			const float patchVel = patchOp.velSens*velocity;

			// Tremolo/Vibrato amount
			voiceOp.vibrato = patchOp.vibrato;
			voiceOp.tremolo = patchOp.tremolo;

			// Amounts/Sensitivity
			voiceOp.pitchEnvAmt = patchOp.pitchEnvAmt;
			voiceOp.feedbackAmt = patchOp.feedbackAmt;

			// Operator env.
			ADSR::Parameters envParams;
			envParams.attack = patchOp.opEnvA;
			envParams.attackLevel = patchOp.opEnvL;
			envParams.decay  = patchOp.opEnvD;
			envParams.release = 0.f;
			envParams.sustainLevel = patchOp.opEnvL*(1.f-patchOp.opEnvD);
			voiceOp.opEnv.Start(envParams, patchVel);
		}

		// Set pitch envelope (like above, but slightly different)
		ADSR::Parameters envParams;
		envParams.attack = s_parameters.pitchA;
		envParams.attackLevel = s_parameters.pitchL;
		envParams.decay = s_parameters.pitchD;
		envParams.release = 0.f;
		envParams.sustainLevel = 0.f; // Sustain at unaltered pitch
		voice.m_pitchEnv.Start(envParams, velocity);

		// Start master ADSR
		voice.m_ADSR.Start(s_parameters.envParams, velocity);

		// Reset & start filters
		LadderFilter *pFilter;
		if (0 == s_parameters.filterType)
			pFilter = s_cleanFilters+iVoice;
		else
			pFilter = s_MOOGFilters+iVoice;

		pFilter->Reset();
		pFilter->Start(s_parameters.filterEnvParams, velocity);

		voice.m_pFilter = pFilter;

		s_vowelFilters[iVoice].Reset();
		
		// Enabled, up counter		
		voice.m_enabled = true;
		++s_active;

		// Store index if wanted
		if (nullptr != request.pIndex)
			*request.pIndex = iVoice;
		
	}

	SFM_INLINE void InstFrontVoice(unsigned iVoice)
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

			const float velocity = request.velocity;

			// Stop main ADSR; this will automatically release the voice
			voice.m_ADSR.Stop(velocity);

			// Additional releasing voice
			++s_releasing;

/*
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
				voice.m_operators[iOp].opEnv.Stop(velocity);

			voice.m_pFilter->Stop(velocity);
*/

			// ^^ Secondary envelopes are not stopped as they have no release
			//    This will probably change in the future

			Log("Voice released: " + std::to_string(request.index));
		}

		// Update active voices
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			DX_Voice &voice = s_DXvoices[iVoice];
			const bool enabled = voice.m_enabled;

			if (true == enabled)
			{
				if (true == voice.m_ADSR.IsIdle())
				{
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
					InstFrontVoice(iVoice);

					Log("Voice triggered: " + std::to_string(iVoice));

					break;
				}
			}
		}

		// Alternatively try to steal a releasing voice
		while (s_voiceReq.size() > 0 && s_releasing > 0)
		{
			float lowestOutput = 1.f;
			unsigned iRelease = -1;

			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];
				if (true == voice.m_ADSR.IsReleasing())
				{
					// Check output level
					const float output = voice.m_ADSR.m_ADSR.getOutput();
					if (lowestOutput >= output)
					{
						// Lowest so far (closer to end of cycle)
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

				InstFrontVoice(iRelease);

				Log("Voice triggered (stolen): " + std::to_string(iRelease) + " with output " + std::to_string(lowestOutput));
			}
			else
				break; // Nothing to steal at all, so bail
		}

		// All release requests have been honoured; note triggers that can't be made are discarded
		s_voiceReleaseReq.clear();

		if (false == s_voiceReq.empty())
			Log("Number of voice requests ignored: " + std::to_string(s_voiceReq.size()));

		s_voiceReq.clear();
	}

	/*
		Parameter update.

		Currently there's only one of these for the Oxygen 49 (+ Arturia BeatStep) driver.

		Ideally all inputs interact on sample level, but this is impractical and only done for a few parameters.

		FIXME:
			- Update parameters every N samples (nuggets).
			- Remove all rogue parameter probes, some of which are:
			  + WinMidi_GetMasterDrive()
			  + ?

		Most tweaking done here on the normalized input parameters should be adapted for the first VST attempt.
	*/

	static void UpdateState_Oxy49_BeatStep(float time)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);

		// Drive
		s_parameters.drive = dBToAmplitude(kDriveHidB)*WinMidi_GetMasterDrive();

		// Tremolo
		s_parameters.tremolo = WinMidi_GetTremolo();

		// LFO form
		s_parameters.LFOform = WinMidi_GetLFOShape();

		// Master ADSR
		s_parameters.envParams.attack  = WinMidi_GetAttack();
		s_parameters.envParams.decay   = WinMidi_GetDecay();
		s_parameters.envParams.release = WinMidi_GetRelease() * kReleaseStretch;
		s_parameters.envParams.sustainLevel = WinMidi_GetSustain();

		// Chorus type
		s_parameters.chorusType = WinMidi_GetChorusType();
		SFM_ASSERT(s_parameters.chorusType <= 1);

		// Vibrato
		const float vibrato = WinMidi_GetVibrato();
		s_parameters.vibrato = vibrato;

		// Note jitter
		s_parameters.noteJitter = WinMidi_GetNoteJitter();

		// Filter
		s_parameters.filterInv = WinMidi_GetFilterInv();
		s_parameters.filterType = WinMidi_GetFilterType();
		s_parameters.filterWet = WinMidi_GetFilterWet();
		s_parameters.filterParams.drive = WinMidi_GetFilterDrive() * dBToAmplitude(kMaxFilterDrivedB);
		s_parameters.filterParams.cutoff = WinMidi_GetCutoff();
		s_parameters.filterParams.resonance = WinMidi_GetResonance();

		// Filter envelope (ADS)
		s_parameters.filterEnvParams.attack = WinMidi_GetFilterA();
		s_parameters.filterEnvParams.decay = WinMidi_GetFilterD();
		s_parameters.filterEnvParams.release = 0.f; // Should never be used (no Stop() call)
		s_parameters.filterEnvParams.sustainLevel = WinMidi_GetFilterS();

		// Delay
		s_parameters.delayWet = WinMidi_GetDelayWet();
		s_parameters.delayRate = WinMidi_GetDelayRate();

		// Pitch env.
		s_parameters.pitchA = WinMidi_GetPitchA();
		s_parameters.pitchD = WinMidi_GetPitchD();
		s_parameters.pitchL = WinMidi_GetPitchL();

		// Vowel filter
		s_parameters.vowelWet = WinMidi_GetVowelWet();
		s_parameters.vowelBlend = WinMidi_GetVowelBlend();
		s_parameters.vowel = WinMidi_GetVowel();

		// Pitch bend
		const float bend = WinMidi_GetPitchBend();
		s_parameters.pitchBend = powf(2.f, bend);

		// Update current operator patch
		const unsigned iOp = WinMidi_GetOperator();
		if (-1 != iOp)
		{
			SFM_ASSERT(iOp >= 0 && iOp < kNumOperators);

			FM_Patch::Operator &patchOp = s_parameters.patch.operators[iOp];

			const bool fixed = patchOp.fixed = WinMidi_GetOperatorFixed();

			if (false == fixed)
			{
				// Volca-style coarse index
				patchOp.coarse = unsigned(WinMidi_GetOperatorCoarse()*31.f);

				// Fine tuning (1 octave like the DX7)
				const float fine = WinMidi_GetOperatorFinetune();
				patchOp.fine = fine;
			}
			else
			{
				// See synth-patch.h for details
				patchOp.coarse = unsigned(WinMidi_GetOperatorCoarse()*3.f); // Index into 4 steps

				const float fine = WinMidi_GetOperatorFinetune();
				patchOp.fine = fine*kFixedFineScale; // Also like the DX7 or Volca FM
			}
	
			// Detune
			patchOp.detune = WinMidi_GetOperatorDetune();

			// Tremolo & vibrato
			patchOp.tremolo = WinMidi_GetOperatorTremolo();
			patchOp.vibrato = WinMidi_GetOperatorVibrato();

			// Velocity sens.
			patchOp.velSens = WinMidi_GetOperatorVelocitySensitivity();

			// Pitch env. amount
			patchOp.pitchEnvAmt = WinMidi_GetOperatorPitchEnvAmount();
			if (-1 == WinMidi_GetOperatorPitchEnvInv())
				patchOp.pitchEnvAmt *= -1.f; // Control direction by inverting the amount

			// Level scaling parameters
			patchOp.levelScaleBP    = WinMidi_GetOperatorLevelScaleBP();
			patchOp.levelScaleLeft  = WinMidi_GetOperatorLevelScaleL();
			patchOp.levelScaleRight = WinMidi_GetOperatorLevelScaleR();
			patchOp.levelScaleExpL  = WinMidi_GetOperatorLevelScaleExpL();
			patchOp.levelScaleExpR  = WinMidi_GetOperatorLevelScaleExpR();

			// Feedback amount
			patchOp.feedbackAmt = WinMidi_GetOperatorFeedbackAmount()*kMaxOperatorFeedback;

			// Envelope
			patchOp.opEnvA = WinMidi_GetOperatorEnvA();
			patchOp.opEnvD = WinMidi_GetOperatorEnvD();
			patchOp.opEnvL = WinMidi_GetOperatorEnvL();
			
			// Amplitude/depth
			patchOp.amplitude = WinMidi_GetOperatorAmplitude();
		}
	}

	/*
		Render function.
	*/

	alignas(16) static float s_voiceBuffers[kMaxVoices][kRingBufferSize];

	// Single tap with MID mix (less pronounced)
	SFM_INLINE void DelayToStereo_2(float mix) 
	{
		s_delayLine.Write(mix);

		const float hundred = kSampleRate/100.f;

		const float tap1 = s_delayLine.Read(hundred);
		const float tap = tap1;

		const float wet = lerpf<float>(mix, tap, s_parameters.delayWet);
		
		const float sweepL = 0.5f + s_delayLFO_L.Sample(0.f);
		const float sweepR = 0.5f + s_delayLFO_R.Sample(0.f);
		const float sweepM = 0.5f + s_delayLFO_M.Sample(0.f);
		
		const float mixL = lerpf<float>(mix, wet, sweepL);
		const float mixR = lerpf<float>(mix, wet, sweepR);
		const float mixM = lerpf<float>(mix, wet, sweepM)*kMinus3dB;

		// FIXME: should the mid. LFO also tell us how it blends in?
		const float L = mixL*kMinus3dB + mixM;
		const float R = mixR*kMinus3dB + mixM;

		s_ringBuf.Write(L);
		s_ringBuf.Write(R);
	}

	// Single tap, just L/R (sounds like Juno 60 Chorus #1)
	SFM_INLINE void DelayToStereo_1(float mix) 
	{
		s_delayLine.Write(mix);

		const float hundred = kSampleRate/100.f;

		const float tap1 = s_delayLine.Read(hundred);
		const float tap = tap1;

		const float wet = lerpf<float>(mix, tap, s_parameters.delayWet);
		
		const float sweepL = 0.5f + s_delayLFO_L.Sample(0.f);
		const float sweepR = 0.5f + s_delayLFO_R.Sample(0.f);

		// I'm "ticking" this oscillator to keep it in sync. but should I?
		const float sweepM = 0.5f + s_delayLFO_M.Sample(0.f);
		
		const float mixL = lerpf<float>(mix, wet, sweepL);
		const float mixR = lerpf<float>(mix, wet, sweepR);

		const float L = mixL;
		const float R = mixR;

		s_ringBuf.Write(L);
		s_ringBuf.Write(R);
	}

	// Returns loudest signal (linear amplitude)
	static float Render(float time)
	{
		float loudest = 0.f;

		// Lock ring buffer
		std::lock_guard<std::mutex> ringLock(s_ringBufMutex);

		// See if there's enough space in the ring buffer to justify rendering
		if (true == s_ringBuf.IsFull())
			return loudest;

		const unsigned available = s_ringBuf.GetAvailable()>>1;
		if (available > kMinSamplesPerUpdate)
			return loudest;

		// Amt. of samples (stereo)
		const unsigned numSamples = (kRingBufferSize>>1)-available;

		// Lock state
		std::lock_guard<std::mutex> stateLock(s_stateMutex);

		// Handle voice logic
		UpdateVoices();

		// Bend delay LFOs
		const float delayBend = powf(2.f, s_parameters.delayRate*kPI);
		s_delayLFO_L.PitchBend(delayBend);
		s_delayLFO_R.PitchBend(delayBend);
		s_delayLFO_M.PitchBend(delayBend);

		const unsigned numVoices = s_active;

		if (0 == numVoices)
		{
			loudest = 0.f;

			// Render silence (we still have to run the effects)
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				// Delay to stereo (FIXME: func. ptr. or smth.)
				if (0 != s_parameters.chorusType)
					DelayToStereo_2(0.f);
				else
					DelayToStereo_1(0.f);
			}
		}
		else
		{
			const float vowelWet = s_parameters.vowelWet;
			const float vowelBlend = s_parameters.vowelBlend;
			const VowelFilter::Vowel vowel = s_parameters.vowel;

			// Render dry samples for each voice (feedback)
			unsigned curVoice = 0;
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				DX_Voice &voice = s_DXvoices[iVoice];
				const float velocity = voice.m_velocity;

				// I'd like to pack a little velocity punch on the loudest struck notes for the vowel filter, emulating
				// (or attempting to) Rhodes pickups overdriving a little
				const float curVowelBlend = kRootHalf*vowelBlend + (1.f-kRootHalf)*vowelBlend;
				
				if (true == voice.m_enabled)
				{
					SFM_ASSERT(nullptr != voice.m_pFilter);
					LadderFilter &filter = *voice.m_pFilter;
					VowelFilter &vowelFilter = s_vowelFilters[iVoice];
				
					const float bend = s_parameters.pitchBend;
					voice.SetPitchBend(bend);

					// Set global vibrato amt.
					voice.m_vibrato.SetAmplitude(s_parameters.vibrato);
	
					float *buffer = s_voiceBuffers[curVoice];
					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						float sample = voice.Sample(s_parameters);

						// Apply vowel filter
						const float formant = vowelFilter.Apply(sample, vowel, curVowelBlend);
						sample = lerpf<float>(sample, formant, vowelWet);

						buffer[iSample] = sample;
					}

					// Filter voice
					filter.SetLiveParameters(s_parameters.filterParams);
					filter.Apply(buffer, numSamples, s_parameters.filterWet /* AKA 'contour' */, s_parameters.filterInv);

					// Apply ADSR (FIXME: slow)
					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						buffer[iSample] *= voice.m_ADSR.Sample();
					}

					++curVoice; // Do *not* use to index anything other than the temporary buffers
				}
			}

			loudest = 0.f;

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

				// Apply drive
				const float drive = WinMidi_GetMasterDrive();
				mix = ultra_tanhf(mix*drive);
				SampleAssert(mix);

				// Delay to stereo (FIXME: func. ptr. or smth.)
				if (0 != s_parameters.chorusType)
					DelayToStereo_2(mix);
				else
					DelayToStereo_1(mix);

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
	const unsigned numSamplesReq = length/sizeof(float)/2;

	std::lock_guard<std::mutex> lock(s_ringBufMutex);
	{
		const unsigned numSamplesAvail = s_ringBuf.GetAvailable()>>1;
		const unsigned numSamples = std::min<unsigned>(numSamplesAvail, numSamplesReq);

		if (numSamplesAvail < numSamplesReq)
		{
//			SFM_ASSERT(false);
			Log("Buffer underrun");
		}

		float *pWrite = reinterpret_cast<float*>(pStream);
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			// FIXME
			*pWrite++ = s_ringBuf.Read();
			*pWrite++ = s_ringBuf.Read();
		}

		s_sampleOutCount += numSamples;
	}
}

/*
	Global (de-)initialization.
*/

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
		s_DXvoices[iVoice].Reset();

	s_delayLine.Reset();

	// 3-phase chorus: L and R at 120 and 240 deg., M at 0 deg.
	s_delayLFO_L.Initialize(kDigiTriangle, kBaseDelayFreq, 0.5f, 0.33f);
	s_delayLFO_R.Initialize(kDigiTriangle, kBaseDelayFreq+0.01f, 0.5f, 0.66f);
	s_delayLFO_M.Initialize(kSine, kBaseDelayFreq, 0.5f, 0.f);
	
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

float Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	// Update state for M-AUDIO Oxygen 49 & Arturia BeatStep
	UpdateState_Oxy49_BeatStep(time);
	
	// Render
	float loudest = Render(time);

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
