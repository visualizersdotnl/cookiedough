
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
	unsigned TriggerNote(float frequency, float velocity)
	{
		SFM_ASSERT(true == InAudibleSpectrum(frequency));

		std::lock_guard<std::shared_mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		const unsigned iVoice = AllocateNote(s_shadowState.m_voices);
		if (-1 == iVoice)
			return -1;

		++state.m_active;

		Voice &voice = state.m_voices[iVoice];

		// FIXE: replace with algorithm-based patch 
		const float carrierFreq = frequency;

		// This is "bro science" at best
		const float amplitude = 0.05f + velocity*0.95f;
	
		voice.m_carrier.Initialize(s_sampleCount, kSine, amplitude, carrierFreq);
		const float ratio = 2.f;
		voice.m_modulator.Initialize(s_sampleCount, 0.f, carrierFreq*ratio, 0.f);
//		voice.m_modulator.Initialize(s_sampleCount, state.modIndex, carrierFreq*ratio, 0.f);

		voice.m_ADSR.Start(s_sampleCount, velocity);

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
					if (envelope.m_sampleOffs+envelope.m_release < s_sampleCount)
					{
						voice.m_enabled = false;
						--state.m_active;
					}
				}
			}
		}
	}

	/*
		Global update.
	*/

	static void Update()
	{
		std::lock_guard<std::shared_mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		UpdateVoices(state);
		
		state.drive = WinMidi_GetMasterDrive()*kMaxOverdrive;
		state.modIndex = WinMidi_GetMasterModulationIndex()*k2PI;	
	}

	/*
		Render function.

		FIXME: fold all into a single loop?
	*/

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

		/*
		static float phase = 0.f;
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			ringBuf.Write(sinf(phase));
			phase += CalculateAngularPitch(440.f);
		}

		return;
		*/

		FM &state = s_renderState;
		Voice *voices = state.m_voices;

		const unsigned numVoices = state.m_active;

		if (0 == numVoices)
		{
			// Silence, but still run (off) the filter
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
				ringBuf.Write(0.f);
		}
		else
		{
			const float drive = WinMidi_GetMasterDrive()*2.f;

			// Render dry samples
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				float dry = 0.f;
				for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
				{
					Voice voice = voices[iVoice];
					if (true == voice.m_enabled)
					{
						const float sample = voice.Sample(s_sampleCount);
						dry = fast_tanhf(dry+sample);
					}
				}

				dry = fast_tanhf(dry*drive);

				ringBuf.Write(dry);

				++s_sampleCount;
			}

		}
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

	// Test hack: Oxygen 49 + SDL2
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
		first = false;
	}
}
