
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

// Win32 MIDI input (M-AUDIO Oxygen 49)
#include "Win-MIDI-in-Oxygen49.h"

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

	static std::mutex s_ringBufLock;
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
	static ImprovedMoogFilter s_improvedFilters[kMaxVoices];
	static MicrotrackerMoogFilter s_MicrotrackerFilters[kMaxVoices];
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

	static std::deque<VoiceRequest> s_voiceReq;
	static std::deque<unsigned> s_voiceReleaseReq;

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

	void ReleaseVoice(unsigned index)
	{
		SFM_ASSERT(-1 != index);

		std::lock_guard<std::mutex> lock(s_stateMutex);
		s_voiceReleaseReq.push_front(index);
	}

	static void InitializeVoice(const VoiceRequest &request, FM &state, unsigned iVoice)
	{
		Voice &voice = state.m_voices[iVoice];

		const float frequency = request.frequency;
		const float velocity = request.velocity;

		// Initialize carrier
		const float amplitude = 0.1f + velocity*dBToAmplitude(kMaxVoicedB);
		voice.m_carrier.Initialize(s_sampleCount, request.form, amplitude, frequency*state.m_modRatioC);

		// Initialize freq. modulator & their pitched counterparts (for pitch bend)
		const float modRatio = state.m_modRatioM+state.m_modDetune;
		voice.m_modulator.Initialize(s_sampleCount, state.m_modIndex, frequency*modRatio, 0.f, state.m_indexLFOFreq);

		// Initialize feedback delay line(s)
		voice.InitializeFeedback();

		// Initialize amplitude modulator (or 'tremolo')
		const float tremolo = state.m_tremolo;
		const float broFrequency = invsqrf(tremolo)*kGoldenRatio; // This is *so* arbitrary
		voice.m_ampMod.Initialize(s_sampleCount, 1.f, broFrequency, kOscPeriod/4.f, 0.f);

		// FIXME: perhaps this should be optional?
		voice.m_oneShot = state.m_loopWaves ?  false : oscIsWavetable(request.form);

		// Set ADSR
		s_ADSRs[iVoice].Start(s_sampleCount, state.m_ADSR, velocity);

		// Get & reset filter
		switch (state.m_curFilter)
		{
		case 1: // Classic MOOG
			voice.m_pFilter = s_improvedFilters+iVoice;
			break;

		case 2: // Tempered variant
			break;

		default:
		case 0: // A bit more expressive (default)
			voice.m_pFilter = s_teemuFilters+iVoice;
			break;
		}

		voice.m_pFilter->Reset();		
		
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
				if (true == voice.m_oneShot && true == voice.m_carrier.HasCycled(s_sampleCount))
					free = true;
				
				// If to be freed enter grace period
				if (true == free)
				{
					voice.m_enabled = false;
					--state.m_active;
				}
			}
		}

		// Process release requests
		for (unsigned index : s_voiceReleaseReq)
		{
			SFM_ASSERT(-1 != index);
			
			Voice &voice = state.m_voices[index];
			s_ADSRs[index].Stop(s_sampleCount);

			Log("Voice released: " + std::to_string(index));
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

		Currently there's only one of these for the Oxygen 49 MIDI driver.

		Ideally all inputs interact on sample level, but this is impractical and only done for the pitch bend wheel.
		Most tweaking done here on the normalized input parameters should be adapted for the first VST attempt.
	*/

	static void UpdateState_Oxygen49(float time)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		// Drive
		state.m_drive = dBToAmplitude(kDriveHidB)*WinMidi_GetMasterDrive();

		// Modulation index
		const float alpha = 1.f/dBToAmplitude(-12.f);
		state.m_modIndex = WinMidi_GetModulationIndex()*alpha;

		// Get ratio from precalculated table (see synth-LUT.cpp)
		const unsigned tabIndex = (unsigned) (WinMidi_GetModulationRatio()*(g_CM_table_size-1));
		state.m_modRatioC = (float) g_CM_table[tabIndex][0];
		state.m_modRatioM = (float) g_CM_table[tabIndex][1];

		// Modulation brightness affects the modulator's oscillator blend (sine <-> triangle)
		state.m_modBrightness = WinMidi_GetModulationBrightness();

		// Modulation index LFO frequency
		const float frequency = WinMidi_GetModulationLFOFrequency();
		state.m_indexLFOFreq = frequency*k2PI*2.f;

		// Modulation detune
		const float detunePot = WinMidi_GetModulationDetune();
		const unsigned detuneIdx = unsigned(detunePot*g_detuneTabSize);
		state.m_modDetune = g_detuneTab[detuneIdx];
		
		// Tremolo
		state.m_tremolo = WinMidi_GetTremolo();

		// Wavetable one-shot yes/no
		state.m_loopWaves = WinMidi_GetLoopWaves();

		// ADSR	
		state.m_ADSR.attack  = WinMidi_GetAttack();
		state.m_ADSR.decay   = WinMidi_GetDecay();
		state.m_ADSR.release = WinMidi_GetRelease()*kGoldenRatio; // Extra room to create pad-like sounds
		state.m_ADSR.sustain = WinMidi_GetSustain();

		// Filter parameters
		state.m_curFilter = WinMidi_GetCurFilter();
		state.m_wetness = Clamp(invsqrf(WinMidi_GetFilterWetness()));
		state.m_filterParams.cutoff = WinMidi_GetFilterCutoff();
		state.m_filterParams.resonance = WinMidi_GetFilterResonance();
		state.m_filterParams.envInfl = WinMidi_GetFilterEnvInfl();

		// Feedback parameters
		state.m_feedback = WinMidi_GetFeedback();
		state.m_feedbackWetness = WinMidi_GetFeedbackWetness();
		state.m_feedbackPitch = -1.f + WinMidi_GetFeedbackPitch()*2.f;
	}

	/*
		Render function.
	*/

	alignas(16) static float s_voiceBuffers[kMaxVoices][kRingBufferSize];

	SFM_INLINE void ProcessDelay(FM &state, float &mix)
	{
		// Process delay
		const float delayPitch = state.m_feedbackPitch;
		const float delayed = s_delayMatrix.Read(delayPitch)*0.66f;
		s_delayMatrix.Write(mix, delayed*state.m_feedback);

		// Apply delay
		mix = mix + state.m_feedbackWetness*delayed;
	}

	// Returns loudest signal (linear amplitude)
	static float Render(float time)
	{
		static float loudest = 0.f;

		std::lock_guard<std::mutex> ringLock(s_ringBufLock);

		if (true == s_ringBuf.IsFull())
			return loudest;

		const unsigned available = s_ringBuf.GetAvailable();
		if (available > kMinSamplesPerUpdate)
			return loudest;

		const unsigned numSamples = kRingBufferSize-available;

		// Lock all state
		std::lock_guard<std::mutex> stateLock(s_stateMutex);

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
				if (true == voice.m_enabled)
				{
					ADSR &envelope = s_ADSRs[iVoice];
					float *buffer = s_voiceBuffers[curVoice];

					for (unsigned iSample = 0; iSample < numSamples; ++iSample)
					{
						const unsigned sampleCount = s_sampleCount+iSample;

						// Probed on this level for accuracy
						const float pitchBend = 2.f*(WinMidi_GetPitchBend()-0.5f);

						const float sample = voice.Sample(sampleCount, pitchBend, state.m_modBrightness, envelope);
						buffer[iSample] = sample;
					}

					voice.m_pFilter->SetParameters(state.m_filterParams);
					voice.m_pFilter->Apply(buffer, numSamples, state.m_wetness, s_sampleCount, envelope);

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
					mix = fast_tanhf(mix+sample);
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

	std::lock_guard<std::mutex> lock(s_ringBufLock);
	{
		const unsigned numSamplesAvail = s_ringBuf.GetAvailable();
		const unsigned numSamples = std::min<unsigned>(numSamplesAvail, numSamplesReq);

		if (numSamplesAvail < numSamplesReq)
		{
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
		s_MicrotrackerFilters[iVoice].Reset();
		s_teemuFilters[iVoice].Reset();
	}

	// Reset voice deques
	s_voiceReq.clear();
	s_voiceReleaseReq.clear();

	// Test: Oxygen 49 driver + SDL2
	const auto numDevs = WinMidi_GetNumDevices();
	const bool midiIn = WinMidi_Start(0);

	// SDL2 audio stream	
	const bool audioOut = SDL2_CreateAudio(SDL2_Callback);

	return true == midiIn && audioOut;
}

void Syntherklaas_Destroy()
{
	WinMidi_Stop();
	SDL2_DestroyAudio();
}

/*
	Frame update (called by Bevacqua).
*/

float Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
//	ClickTest(time, 1.f);

	float loudest = 0.f;

	// Update state for M-AUDIO Oxygen 49
	UpdateState_Oxygen49(time);
	
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
