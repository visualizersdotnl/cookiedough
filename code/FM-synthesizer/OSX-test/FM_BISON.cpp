
/*
	Syntherklaas FM presents 'FM. BISON'
	(C) syntherklaas.org, a subsidiary of visualizers.nl

	OSX HACK!
*/

// Oh MSVC and your well-intentioned madness!
#define _CRT_SECURE_NO_WARNINGS

// Easily interchangeable with POSIX-variants or straight assembler, though so complication
// may arise with all the different (system/API) threads calling
#include <mutex>
#include <shared_mutex>
#include <atomic>

#include "FM_BISON.h"
#include "synth-LFOs.h"
#include "synth-midi.h"
#include "synth-oscillators.h"
#include "synth-state.h"
#include "synth-MOOG-ladder.h"
// #include "synth-ringbuffer.h"

// Win32 MIDI input (specialized for the Oxygen 49)
// #include "windows-midi-in.h"

namespace SFM
{
	/*
		Global sample counts.

		FIXME: 
			- Account for wrapping.
			- Atomic can be dropped later.
	*/

//	static std::atomic<unsigned> s_sampleCount = 0;
//	static std::atomic<unsigned> s_sampleOutCount = 0;
	static unsigned s_sampleCount = 0, s_sampleOutCount = 0;

	/*
		FM modulator.
	*/

	void FM_Modulator::Initialize(float index, float frequency)
	{
		m_index = index;
		m_pitch = CalcSinPitch(frequency);
		m_sampleOffs = s_sampleCount;
	}

	float FM_Modulator::Sample(const float *pEnv)
	{
		const unsigned sample = s_sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch;

		float envelope = 1.f;
		if (pEnv != nullptr)
		{
			const unsigned index = sample & (kPeriodLength-1);
			envelope = pEnv[index];
		}

		const float modulation = oscSine(phase)*kTabToRad; // FIXME: try other oscillators (not without risk of noise, of course)
		return envelope*m_index*modulation;
	}

	/*
		FM carrier.
	*/

	void FM_Carrier::Initialize(Waveform form, float amplitude, float frequency)
	{
		m_form = form;
		m_amplitude = amplitude;
		m_pitch = CalcSinPitch(frequency);
		m_sampleOffs = s_sampleCount;
		m_numHarmonics = GetCarrierHarmonics(frequency);
	}

	// FIXME: kill switch!
	float FM_Carrier::Sample(float modulation)
	{
		const unsigned sample = s_sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch;

		float signal = 0.f;
		switch (m_form)
		{
		case kSine:
			signal = oscSine(phase+modulation);
			break;

		case kSaw:
			signal = oscSaw(phase+modulation, m_numHarmonics);
			break;

		case kSquare:
			signal = oscSquare(phase+modulation, m_numHarmonics);
			break;

		case kDirtySaw:
			signal = oscDirtySaw(phase+modulation);
			break;

		case kDirtyTriangle:
			signal = oscDirtyTriangle(phase+modulation);
			break;
		}

		return m_amplitude*signal;
	}

	/*
		Global & shadow state.
		
		The shadow state may be updated after acquiring the lock (FIXME: use cheaper solution).
		This should be done for all state that is set on a push-basis.

		Things to remember:
			- Currently the synthesizer itself just uses a single thread; when expanding, things will get more complicated.
			- Pulling state is fine, for now.

		This creates a tiny additional lag between input and output, but an earlier commercial product has shown this
		to be negligible so long as the update buffer's size isn't too large.
	*/

//	static std::shared_mutex s_stateMutex;
	static FM s_shadowState;
	static FM s_renderState;

	/*
		MIDI-driven filter setup.
		Filter state is not included in the global state for now as it pulls the values rather than pushing them.
	*/

	static void UpdateFilterSettings()
	{
		float testCut, testReso;
		testCut = 0.9f; // WinMidi_GetCutoff();
		testReso = 0.1f;  // WinMidi_GetResonance();

		MOOG::SetCutoff(testCut*1000.f);
		MOOG::SetResonance(testReso*kPI);
		MOOG::SetDrive(1.f);
	}

	/*
		Note (voice) logic.
	*/

	// - First fit (FIXME: check lifetime, volume, pitch, just try different heuristics)
	// - Assumes being used on shadow parameters (so lock!)
	SFM_INLINE unsigned AllocNote(FM_Voice *voices)
	{
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			const bool enabled = voices[iVoice].enabled;
			if (false == enabled)
			{
				return iVoice;
			}
		}

		return -1;
	}

	// Returns voice index, if available
	unsigned TriggerNote(unsigned midiIndex)
	{
		FM &state = s_shadowState;

//		std::lock_guard<std::shared_mutex> lock(s_stateMutex);

		const unsigned iVoice = AllocNote(s_shadowState.voices);
		if (-1 == iVoice)
			return -1;

		++state.active;

		FM_Voice &voice = state.voices[iVoice];

		// FIXME: adapt to patch when it's that time
		const float carrierFreq = g_midiToFreqLUT[midiIndex];
		voice.carrier.Initialize(kDirtySaw, kMaxVoiceAmplitude, carrierFreq);
		const float ratio = 4.f/1.f;
		const float CM = carrierFreq*ratio;
		voice.modulator.Initialize(0.f /* LFO! */, CM); // These parameters mean a lot
		voice.envelope.Start(s_sampleCount);

		voice.enabled = true;

		return iVoice;
	}

	void ReleaseNote(unsigned iVoice)
	{
		FM &state = s_shadowState;

//		std::lock_guard<std::shared_mutex> lock(s_stateMutex);
		FM_Voice &voice = state.voices[iVoice];
		voice.envelope.Stop(s_sampleCount);
	}

	// FIXME: for now this is a hack that checks if enabled voices are fully released, and frees them,
	//        but I can see this function have more use later on (like, not at 07:10 AM)
	void UpdateNotes()
	{
//		std::lock_guard<std::shared_mutex> lock(s_stateMutex);

		FM &state = s_shadowState;

		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			FM_Voice &voice = state.voices[iVoice];
			const bool enabled = voice.enabled;
			if (true == enabled)
			{
				if (ADSR::kRelease == voice.envelope.m_state)
				{
					if (voice.envelope.m_sampleOffs+voice.envelope.m_release < s_sampleCount)
					{
						voice.enabled = false;
						--state.active;
					}
				}
			}
		}
	}

	/*
		ADSR implementation.

		FIXME:
			- MIDI parameters
			- Non-linear slopes
			- Ronny's idea (note velocity scales attack and release time and possibly curvature)
			- Make these settings global and only use a thin copy of this object (that takes an operator X) with each voice
	*/

	void ADSR::Start(unsigned sampleOffs)
	{
		m_sampleOffs = sampleOffs;
		m_attack = kSampleRate/2;
		m_decay = 0;
		m_release = kSampleRate/4;
		m_sustain = 0.8f;
		m_state = kAttack;
	}

	void ADSR::Stop(unsigned sampleOffs)
	{
		// Always use current amplitude for release
		m_sustain = ADSR::Sample();

		m_sampleOffs = sampleOffs;
		m_state = kRelease;
	}

	float ADSR::Sample()
	{
		const unsigned sample = s_sampleCount-m_sampleOffs;

		float amplitude = 0.f;
		switch (m_state)
		{
		case kAttack:
			{
				if (sample < m_attack)
				{
					const float delta = m_sustain/m_attack;
					amplitude = delta*sample;
				}
				else 
				{
					amplitude = m_sustain;
					m_state = kSustain;
				}
			}
			break;

		case kDecay: // FIXME
		case kSustain:
			amplitude = m_sustain;			
			break;

		case kRelease:
			{
				if (sample < m_release)
				{
					const float delta = m_sustain/m_release;
					amplitude = m_sustain - (delta*sample);
				}
				else 
				{
					amplitude = 0.f; 
				}
			}
			break;
		}

		return amplitude;
	}

	/*
		Render function.
		FIXME: crude and weird impl., optimize.
	*/

	SFM_INLINE void CopyShadowToRenderState()
	{
//		std::lock_guard<std::shared_mutex> stateCopyLock(s_stateMutex);
		s_renderState = s_shadowState;
	}

	static void Render(float *pDest, unsigned numSamples)
	{
		CopyShadowToRenderState();	

		FM &state = s_renderState;
		FM_Voice *voices = state.voices;

		const unsigned numVoices = state.active;

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
					FM_Voice &voice = voices[iVoice];
					if (true == voice.enabled)
					{
						const float sample = voice.Sample();
						dry += sample;
					}
				}

				const float clipped = clampf(-1.f, 1.f, dry);
				pDest[iSample] = atanf(clipped); // FIXME: atanf() LUT

				++s_sampleCount;
			}
		}

//		const float wetness = WinMidi_GetFilterMix();
		const float wetness = 0.314f;
		MOOG::SetDrive(1.f); // FIXME: use *any* clipping?
		MOOG::Filter(pDest, numSamples, wetness);
	}

	// FIXME: hack for now to initialize an indefinite voice and start the stream.
	static bool s_isReady = false;

}; // namespace SFM

using namespace SFM;

/*
	Global (de-)initialization.
*/

bool Syntherklaas_Create()
{
	// Calc. LUTs
	CalculateSinLUT();
	CalcMidiToFreqLUT();

	// Reset shadow FM state & filter(s)
	s_shadowState.Reset();
	MOOG::Reset();

	// Reset sample count
	s_sampleCount = 0;
	s_sampleOutCount = 0;

	return true;

//	const auto numDevs = WinMidi_GetNumDevices();
//	const bool midiIn = WinMidi_Start(0);

//	return true == midiIn;
}

void Syntherklaas_Destroy()
{
//	WinMidi_Stop();
}

/*
	Render function for Kurt Bevacqua codebase.
*/

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	if (false == s_isReady)
	{
		// Start blasting!
//		Audio_Start_Stream(0);
		s_isReady = true;
	}

	/*
		FIXME: just shit out one sample for 8 seconds, fix real output!
	*/
	
	const float duration = 8.f;
	const unsigned numSamples = kSampleRate*duration;

	float render[numSamples];
	
	UpdateFilterSettings();

	const unsigned index1 = TriggerNote(64);
	const unsigned index2 = TriggerNote(66);
	const unsigned index3 = TriggerNote(68);

	const unsigned release = kSampleRate;

	unsigned writeIdx = 0;

	for (unsigned iSample = 0; iSample < numSamples-release; ++iSample)
	{		
		UpdateNotes();
		Render(render+writeIdx++, 1);
	}

	ReleaseNote(index1);
	ReleaseNote(index2);
	ReleaseNote(index3);

	for (unsigned iSample = 0; iSample < release; ++iSample)
	{		
		UpdateNotes();
		Render(render+writeIdx++, 1);
	}
	
	FILE *hFile;
	hFile = fopen("test-out.raw", "wb");
	fwrite(render, sizeof(float), numSamples, hFile);
	fclose(hFile);

	// Check for stream starvation
//	SFM_ASSERT(true == Audio_Check_Stream());
}

/*
	Stream callback (FIXME)
*/

#if 0

DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser)
{
	unsigned numSamplesReq = length/sizeof(float);
	numSamplesReq = std::min<unsigned>(numSamplesReq, kRingBufferSize);

//	Little test to see if this function is working using BASS' current configuration.
//	float pitch = CalcSinPitch(440.f);
//	float *pWrite = (float *) pDest;
//	for (auto iSample = 0; iSample < numSamplesReq; ++iSample)
//	{
//		pWrite[iSample] = SFM::oscSine(s_sampleCount++ * pitch);
//	}

	// FIXME: move rendering to Syntherklaas_Render()
	UpdateNotes(); // Update (shadow) voices (or notes if you will)
	UpdateFilterSettings(); // Pull-only!
	Render((float*) pDest, numSamplesReq);
	s_sampleOutCount += numSamplesReq;

	return numSamplesReq*sizeof(float);
}

#endif