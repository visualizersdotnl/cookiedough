
/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl
*/

// Shut it, MSVC.
#define _CRT_SECURE_NO_WARNINGS

// Easily interchangeable with POSIX-variants or straight assembler
#include <mutex>
#include <shared_mutex>
#include <atomic>

#include "FM_BISON.h"
#include "synth-midi.h"
#include "synth-state.h"
#include "synth-audio-out.h"
#include "synth-ringbuffer.h"
#include "synth-filter.h" 

// Win32 MIDI input (M-AUDIO Oxygen 49)
#include "Win-MIDI-in-Oxygen49.h"

// SDL2 audio output
#include "SDL2-audio-out.h"

namespace SFM
{
	/*
		Global sample counts.

		FIXME: drop atomic property when rendering from main thread!
	*/

	static std::atomic<unsigned> s_sampleCount = 0;
	static std::atomic<unsigned> s_sampleOutCount = 0;


	/*
		Global & shadow state.
		
		The shadow state may be updated after acquiring the lock.
		This should be done for all state that is set on a push-basis.

		Things to remember:
			- Currently the synthesizer itself just uses a single thread; when expanding, things will get more complicated.
			- Pulling state is fine, for now.

		This creates a tiny additional lag between input and output, but an earlier commercial product has shown this
		to be negligible so long as the update buffer's size isn't too large.
	*/

	static std::shared_mutex s_stateMutex;

	static FM s_shadowState;
	static FM s_renderState;
	
	/*
		API + logic.
	*/

	SFM_INLINE unsigned AllocateNote(Voice *voices)
	{
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			const bool enabled = voices[iVoice].m_enabled;
			if (false == enabled)
			{
				return iVoice;
			}
		}

		return -1;
	}

	// Returns voice index, if available
	unsigned TriggerNote(Waveform form, float frequency, float velocity)
	{
		SFM_ASSERT(true == InAudibleSpectrum(frequency));

		std::lock_guard<std::shared_mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		const unsigned iVoice = AllocateNote(s_shadowState.m_voices);
		if (-1 == iVoice)
			return -1;

		++state.m_active;

		Voice &voice = state.m_voices[iVoice];

		const float carrierFreq = frequency;
		
		// She blinded him with "bro science"
		float amplitude = 0.1f*kMaxVoiceAmplitude + 0.9f*velocity*kMaxVoiceAmplitude;

		voice.m_carrier.Initialize(s_sampleCount, form, amplitude, carrierFreq);

		const float ratio = state.m_modRatio;
		voice.m_modulator.Initialize(s_sampleCount, state.m_modIndex, carrierFreq*ratio, 0.f);

		voice.m_ADSR.Start(s_sampleCount, state.m_ADSR, velocity);

		voice.m_enabled = true;

		return iVoice;
	}

	void ReleaseVoice(unsigned iVoice)
	{
		FM &state = s_shadowState;

		std::lock_guard<std::shared_mutex> lock(s_stateMutex);
		Voice &voice = state.m_voices[iVoice];
		
		voice.m_ADSR.Stop(s_sampleCount);
	}

	static void UpdateVoices(FM &state)
	{
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			Voice &voice = state.m_voices[iVoice];
			const bool enabled = voice.m_enabled;
			if (true == enabled)
			{
				ADSR &envelope = voice.m_ADSR;
				if (true == envelope.m_releasing)
				{
					if (envelope.m_sampleOffs+envelope.m_parameters.release < s_sampleCount)
					{
						voice.m_enabled = false;
						--state.m_active;
					}
				}
			}
		}
	}
	
	/*
		Master filter.
	*/

	static MicrotrackerMoogFilter s_moogFilter;

	/*
		Global update.
	*/

	static void Update()
	{
		std::lock_guard<std::shared_mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		UpdateVoices(state);
		
		// Get state from Oxygen 49 driver (FIXME: test)

		state.m_drive = WinMidi_GetMasterDrive()*kMaxOverdrive;
		state.m_modIndex = WinMidi_GetMasterModulationIndex();
		state.m_modRatio = WinMidi_GetMasterModulationRatio()*4.f;
	
		state.m_ADSR.attack = unsigned(WinMidi_GetMasterAttack()*kSampleRate);
		state.m_ADSR.decay = unsigned(WinMidi_GetMasterDecay()*kSampleRate);
		
		// Allow release to be twice as long for a nice lingering effect which in turn can be modulated
		state.m_ADSR.release = unsigned(WinMidi_GetMasterRelease()*kSampleRate*2.f);
		
		state.m_ADSR.sustain = WinMidi_GetMasterSustain();

		s_moogFilter.SetCutoff(WinMidi_GetFilterCutoff());
		s_moogFilter.SetResonance(WinMidi_GetFilterResonance());
		state.m_wetness = WinMidi_GetFilterWetness();

		// Calculate modulation index envelope
		const float tilt = -1.f + 2.f*WinMidi_GetMasterModLFOTilt();
		const float curve = WinMidi_GetMasterModLFOPower()*k2PI;
		const float frequency = kEpsilon + kPI*WinMidi_GetMasterModLFOFrequency();
		CalculateCosineTiltEnvelope(state.m_modIndexLFO, kOscPeriod, tilt, curve, frequency);
	}

	/*
		Render function.
	*/

 	alignas(16) static float s_renderBuf[kRingBufferSize];

	SFM_INLINE void CopyShadowToRenderState()
	{
		std::lock_guard<std::shared_mutex> stateCopyLock(s_stateMutex);
		s_renderState = s_shadowState;
	}

	static void Render(FIFO &ringBuf)
	{
		CopyShadowToRenderState();
		// ^^ Maybe we can share this mutex.

		const unsigned numSamples = ringBuf.GetFree();

		FM &state = s_renderState;
		Voice *voices = state.m_voices;

		const unsigned numVoices = state.m_active;

		if (0 == numVoices)
		{
			// Silence, but still run (off) the filter
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
				s_renderBuf[iSample] = 0.f;
		}
		else
		{
			// Render dry samples
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				float dry = 0.f;
				for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
				{
					Voice voice = voices[iVoice];
					if (true == voice.m_enabled)
					{
						const float sample = voice.Sample(s_sampleCount, state.m_modIndexLFO);
						SFM_ASSERT(true == FloatCheck(sample));
						dry = fast_tanhf(dry + sample);
					}
				}

				dry = fast_tanhf(dry*state.m_drive);
				s_renderBuf[iSample] = dry;
				++s_sampleCount;

			}

			s_moogFilter.Apply(s_renderBuf, numSamples, state.m_wetness);
		}

		// FIXME: optimize
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			ringBuf.Write(s_renderBuf[iSample]);
	}

}; // namespace SFM

using namespace SFM;

/*
	Ring buffer + lock.

	FIXME: a lot here can be optimized.
*/

static FIFO s_ringBuf;
static std::mutex s_bufLock;

/*
	SDL2 stream callback.
*/

static void SDL2_Callback(void *pData, uint8_t *pStream, int length)
{
	const unsigned numSamplesReq = length/sizeof(float);

	s_bufLock.lock();

	const unsigned numSamplesAvail = s_ringBuf.GetAvailable();
	const unsigned numSamples = std::min<unsigned>(numSamplesAvail, numSamplesReq);

	float *pWrite = reinterpret_cast<float*>(pStream);
	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		*pWrite++ = s_ringBuf.Read();
	}

	s_bufLock.unlock();

	s_sampleOutCount += numSamples;
}

/*
	Global (de-)initialization.
*/

bool Syntherklaas_Create()
{
	// Calc. LUTs
	CalculateLUTs();
	CalculateMidiToFrequencyLUT();

	// Reset shadow FM state
	s_shadowState.Reset();

	// Reset sample count
	s_sampleCount = 0;
	s_sampleOutCount = 0;

	// Test (FIXME): Oxygen 49 driver + SDL2
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
	Render function for Kurt Bevacqua codebase.
*/

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	Update();

	s_bufLock.lock();
	Render(s_ringBuf);
	s_bufLock.unlock();

	static bool first = false;
	if (false == first)
	{
		SDL2_StartAudio();
		first = true;

		// Let the world know
		Log("FM. BISON is up & running!");
	}
}
