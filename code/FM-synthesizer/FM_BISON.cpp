
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

// Win32 MIDI input (M-AUDIO Oxygen 49)
#include "Win-MIDI-in-Oxygen49.h"

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
	unsigned TriggerNote(float frequency)
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
		voice.m_carrier.Initialize(s_sampleCount, kSine, kMaxVoiceAmplitude, carrierFreq);
		const float ratio = 4.f;
		const float CM = carrierFreq*ratio;
		voice.m_modulator.Initialize(s_sampleCount, 2.f, CM, 0.f);
		voice.m_ADSR.Start(s_sampleCount, 1.f /* FIXME */);

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

	// To be called every time before rendering
	static void UpdateVoices()
	{
		std::lock_guard<std::shared_mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

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
		Render function.

		FIXME: fold all into a single loop?
	*/

	SFM_INLINE void CopyShadowToRenderState()
	{
		std::lock_guard<std::shared_mutex> stateCopyLock(s_stateMutex);
		s_renderState = s_shadowState;
	}

	static void Render(float *pDest, unsigned numSamples)
	{
		CopyShadowToRenderState();	

		FM &state = s_renderState;
		Voice *voices = state.m_voices;

		const unsigned numVoices = state.m_active;

		if (0 == numVoices)
		{
			// Silence, but still run (off) the filter
			memset(pDest, 0, numSamples*sizeof(float));
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
						const float sample = voice.Sample(s_sampleCount);
						dry += sample;
					}
				}

				pDest[iSample] = Clamp(dry);

				++s_sampleCount;
			}

		}
	}

	// FIXME
	static bool s_isReady = false;

}; // namespace SFM

using namespace SFM;

/*
	Global (de-)initialization.
*/

bool Syntherklaas_Create()
{
	// Calc. LUTs
	CalculateLUT();
	CalculateMidiToFrequencyLUT();

	// Reset shadow FM state
	s_shadowState.Reset();

	// Reset sample count
	s_sampleCount = 0;
	s_sampleOutCount = 0;

	const auto numDevs = WinMidi_GetNumDevices();
	const bool midiIn = WinMidi_Start(0);

	return true == midiIn;
}

void Syntherklaas_Destroy()
{
	WinMidi_Stop();
}

/*
	Render function for Kurt Bevacqua codebase.
*/

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	if (false == s_isReady)
	{
		// Start streaming!
		Audio_Start_Stream(0);
		s_isReady = true;
	}

	// Check for stream starvation
	SFM_ASSERT(true == Audio_Check_Stream());
}

/*
	Stream callback
*/

DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser)
{
	unsigned numSamplesReq = length/sizeof(float);
	numSamplesReq = std::min<unsigned>(numSamplesReq, kRingBufferSize);

	UpdateVoices(); 
	Render((float*) pDest, numSamplesReq);
	s_sampleOutCount += numSamplesReq;

	return numSamplesReq*sizeof(float);
}
