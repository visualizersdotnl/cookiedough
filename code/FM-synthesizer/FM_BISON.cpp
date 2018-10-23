
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
	*/

 	static std::atomic<unsigned> s_sampleCount = 0;
	static std::atomic<unsigned> s_sampleOutCount = 0;

	/*
		Global state.
		
		The shadow state may be updated after acquiring the lock.
		This should be done for all state that is set on a push-basis.

		Things to remember:
			- Currently the synthesizer itself just uses a single thread; when expanding, things will get more complicated.
			- Pulling state is fine, for now.

		This creates a tiny additional lag between input and output, but an earlier commercial product has shown this
		to be negligible so long as the update buffer's size isn't too large.
	*/

	static std::mutex s_stateMutex;

	static FM s_shadowState;
	static FM s_renderState;

	/*
		ADSRs & filters (one for each voice).

		These are not part of the state object since they alter their state
		and aren't directly affected by synth. driver settings.
	*/

	static ADSR s_ADSRs[kMaxVoices];
	static MicrotrackerMoogFilter s_filters[kMaxVoices];
		
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

		Log("Insufficient polyphony!");
		return -1;
	}

	// Returns voice index, if available
	unsigned TriggerNote(Waveform form, float frequency, float velocity)
	{
		SFM_ASSERT(true == InAudibleSpectrum(frequency));

		std::lock_guard<std::mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		const unsigned iVoice = AllocateNote(s_shadowState.m_voices);
		if (-1 == iVoice)
			return -1;

		++state.m_active;

		Voice &voice = state.m_voices[iVoice];
		
		// She blinded him with "bro science"
		const float carrierFreq = frequency;
		float amplitude = velocity*dBToAmplitude(kMaxVoicedB);
		voice.m_carrier.Initialize(s_sampleCount, form, amplitude, carrierFreq);

		// Set modulator
		const float ratio = state.m_modRatio;
		voice.m_modulator.Initialize(s_sampleCount, state.m_modIndex, carrierFreq*ratio, 0.f, state.m_indexLFOParams);

		// Set ADSR
		s_ADSRs[iVoice].Start(s_sampleCount, state.m_ADSR, velocity);

		// And we're live!
		voice.m_enabled = true;

		return iVoice;
	}

	void ReleaseVoice(unsigned iVoice)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		Voice &voice = state.m_voices[iVoice];
		s_ADSRs[iVoice].Stop(s_sampleCount);
	}

	static void UpdateVoices(FM &state)
	{
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			Voice &voice = state.m_voices[iVoice];
			const bool enabled = voice.m_enabled;
			if (true == enabled)
			{
				ADSR &envelope = s_ADSRs[iVoice];
				if (true == envelope.m_releasing)
				{
					if (envelope.m_sampleOffs+envelope.m_parameters.release < s_sampleCount)
					{
						SFM_ASSERT(envelope.Sample(s_sampleCount) == 0.f);
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
		std::lock_guard<std::mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		UpdateVoices(state);
		
		// Get state from Oxygen 49 driver (lots of "bro science")
		// Sounds OK now, 23/10/2018, 01:45

		const float alpha = 1.f/dBToAmplitude(-12.f);

		state.m_drive = WinMidi_GetMasterDrive()*kMaxOverdrive;
		state.m_modIndex = WinMidi_GetMasterModulationIndex()*alpha;
		state.m_modRatio = ceilf(WinMidi_GetMasterModulationRatio()*15.f);
	
		state.m_ADSR.attack = unsigned(WinMidi_GetMasterAttack()*kSampleRate);
		state.m_ADSR.decay = unsigned(WinMidi_GetMasterDecay()*kSampleRate);
		const unsigned release = unsigned(WinMidi_GetMasterRelease()*kSampleRate);
		state.m_ADSR.release = release;
		state.m_ADSR.sustain = WinMidi_GetMasterSustain();

		state.m_filterParams.cutoff = WinMidi_GetFilterCutoff();
		state.m_filterParams.resonance = WinMidi_GetFilterResonance();
		state.m_wetness = WinMidi_GetFilterWetness();

		// Modulation index envelope
		const float tilt = WinMidi_GetMasterModLFOTilt();
		const float curve = WinMidi_GetMasterModLFOPower();
		const float frequency = WinMidi_GetMasterModLFOFrequency();
		state.m_indexLFOParams.tilt = tilt;
		state.m_indexLFOParams.curve = fabsf(curve*12.f);
		state.m_indexLFOParams.frequency = fabsf(frequency*6.f);
	}

	/*
		Render function.
	*/

 	alignas(16) static float s_renderBuf[kRingBufferSize];
	alignas(16) static float s_voiceBuffers[kMaxVoices][kRingBufferSize];

	SFM_INLINE void CopyShadowToRenderState()
	{
		std::lock_guard<std::mutex> stateCopyLock(s_stateMutex);
		s_renderState = s_shadowState;
	}

	static void Render(FIFO &ringBuf)
	{
		CopyShadowToRenderState();

		const unsigned numSamples = ringBuf.GetFree();

		FM &state = s_renderState;
		Voice *voices = state.m_voices;

		const unsigned numVoices = state.m_active;

		if (0 == numVoices)
		{
			// Silence, but still run (off) the filter
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				ringBuf.Write(0.f);
			}
		}
		else
		{
			// Render dry samples for each voice
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
						const float sample = voice.Sample(s_sampleCount+iSample, envelope);
						SFM_ASSERT(true == FloatCheck(sample));
						buffer[iSample] = sample;
					}

					// Apply filter
					s_filters[curVoice].Set(state.m_filterParams);
					s_filters[curVoice].Apply(buffer, numSamples, state.m_wetness, s_sampleCount, envelope);

					++curVoice;
				}
			}

			// Mix voices
			for (unsigned iSample = 0; iSample < numSamples; ++iSample)
			{
				float mix = 0.f;
				for (unsigned iVoice = 0; iVoice < numVoices; ++iVoice)
				{
					const float sample = s_voiceBuffers[iVoice][iSample];
					mix = fast_tanhf(mix + sample);
				}

				mix = fast_tanhf(mix*state.m_drive);
				ringBuf.Write(mix);
			}
		}

		s_sampleCount += numSamples;
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
	{
		const unsigned numSamplesAvail = s_ringBuf.GetAvailable();
		const unsigned numSamples = std::min<unsigned>(numSamplesAvail, numSamplesReq);

		float *pWrite = reinterpret_cast<float*>(pStream);
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			*pWrite++ = s_ringBuf.Read();
		}

		s_sampleOutCount += numSamples;
	}
	s_bufLock.unlock();
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

	// Reset shadow FM state
	s_shadowState.Reset(s_sampleCount);

	// Reset global state
	for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
	{
		s_filters[iVoice].Reset();
	}

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
	{
		Render(s_ringBuf);
	}
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
