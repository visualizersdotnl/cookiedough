
/*
	Syntherklaas FM presents 'FM. BISON', probably the best FM syntheszier in the world
	(C) syntherklaas.org, a subsidiary of visualizers.nl
*/

// Oh MSVC and your well-intentioned madness!
#define _CRT_SECURE_NO_WARNINGS

// Easily interchangeable with POSIX-variants or straight assembler, though so complication
// may arise with all the different (system/API) threads calling
#include <mutex>
#include <shared_mutex>
#include <atomic>

#include "FM_BISON.h"
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
	*/

//	FIXME: drop atomic when safe.
//	static std::atomic<unsigned> s_sampleCount = 0;
//	static std::atomic<unsigned> s_sampleOutCount = 0;
	static unsigned s_sampleCount = 0, s_sampleOutCount = 0;

	/*
		FM modulator.
	*/

	void FM_Modulator::Initialize(float index, float frequency, float phaseShift)
	{
		m_index = index;
		m_pitch = CalcSinLUTPitch(frequency);
		m_sampleOffs = s_sampleCount;
		m_phaseShift = (phaseShift*kSinLUTPeriod)/k2PI;
	}

	float FM_Modulator::Sample()
	{
		const unsigned sample = s_sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch + m_phaseShift;
		
		// FIXME: try other oscillators (not without risk of noise, of course)
		const float modulation = oscSine(phase); 

		return m_index*modulation;
	}

	/*
		Vorticity.
	*/

	void Vorticity::Initialize(unsigned sampleOffs, float acceleration, float wetness)
	{
		SFM_ASSERT(acceleration >= 0.f);
		SFM_ASSERT(fabsf(wetness) <= 1.f);

		m_sampleOffs = sampleOffs;
		m_acceleration = acceleration;
	
		m_pitch = CalcSinLUTPitch(kCommonStrouhal);
		m_pitchMod = CalcSinLUTPitch(acceleration);
		m_wetness = wetness;
	}

	void Vorticity::SetStrouhal(float sheddingFreq, float diameter, float velocity)
	{
		SFM_ASSERT(velocity > 0.f);
		const float S = (sheddingFreq*diameter)/velocity;
		m_pitch = CalcSinLUTPitch(S);
	}

	float Vorticity::Sample(float input)
	{
		// FIXME: add subtle noise at higher frequency
		const unsigned sample = s_sampleCount-m_sampleOffs;
		const float angle = sample*m_pitch;
		const float modAngle = sample*m_pitchMod;
		const float modulation = lutcosf(angle + kLinToSinLUT*lutsinf(modAngle));
		return lerpf<float>(input, input*modulation, m_wetness);
	}

	/*
		FM carrier.
	*/

	void FM_Carrier::Initialize(Waveform form, float amplitude, float frequency)
	{
		m_form = form;
		m_amplitude = amplitude;
		m_pitch = CalcSinLUTPitch(frequency);
		m_angularPitch = CalcAngularPitch(frequency);
		m_sampleOffs = s_sampleCount;
		m_numHarmonics = GetCarrierHarmonics(frequency);
	}

	float FM_Carrier::Sample(float modulation)
	{
		const unsigned sample = s_sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch;

		// Convert modulation to LUT period
		modulation *= kLinToSinLUT;

		// FIXME: get rid of switch!
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
		ADSR implementation.

		This ADSR envelope has some non-standard properties, being:
			- Attack and release is influenced by note velocity, as well as carrier pitch.
			- Curves are a little unorthodox.
			- Decay should be quick!

		FIXME:
			- Ronny's idea: note velocity scales attack and release time and possibly curvature
			- MIDI parameters (velocity, ADSR on faders)
			- Make these settings global and only use a thin copy of this object (that takes an operator X) with each voice
	*/

	void ADSR::Start(unsigned sampleOffs, float velocity)
	{
		SFM_ASSERT(fabsf(velocity) <= 1.f);

		m_sampleOffs = sampleOffs;
		m_velocity = velocity;

		// FIXME: feed by MIDI (or another source) ,can also not be zero in some cases!
		m_attack = kSampleRate/2; // 0.5s
		m_decay = kSampleRate/4;  // 0.25s
		m_release = kSampleRate;  // 1s
		m_sustain = 0.9f; // FIXME: logarithmic scale, more intuitive?
		m_releasing = false;
	}

	void ADSR::Stop(unsigned sampleOffs)
	{
		// Always use current amplitude for release
		m_sustain = ADSR::Sample();

		m_sampleOffs = s_sampleCount;
		m_releasing = true;
	}

	float ADSR::Sample()
	{
		/* const */ unsigned sample = s_sampleCount-m_sampleOffs;

		float amplitude = m_sustain;

		if (false == m_releasing)
		{
			if (sample <= m_attack)
			{
				// Build up to full attack (exponential)
				const float step = 1.f/m_attack;
				const float delta = sample*step;
				amplitude = delta*delta;
			}
			else if (sample > m_attack && sample <= m_attack+m_decay)
			{
				// Decay to sustain (exponential)
				sample -= m_attack;
				const float step = 1.f/m_decay;
				const float delta = sample*step;
				amplitude = lerpf(1.f, m_sustain, delta*delta); 			
			}
		}
		else
		{
			// Sustain level and sample offset are adjusted on NOTE_OFF (again, exponential)
			if (sample <= m_release)
			{
				const float step = 1.f/m_release;
				const float delta = sample*step;
				amplitude = lerpf<float>(m_sustain, 0.f, delta*delta);
			}
		}

		return amplitude;
	}

	/*
		FM voice.
	*/

	// FIXME: 
	//  - Expand with a patch or algorithm selection of sorts
	float FM_Voice::Sample()
	{
		const float modulation = m_modulator.Sample();
		const float ampEnv = m_ADSR.Sample();
		float sample = m_carrier.Sample(modulation)*ampEnv;

		if (true == m_ADSR.m_releasing)
		{
			sample = m_vorticity.Sample(sample);
		}

		return sample;
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

	static void UpdateFilterParameters()
	{
		float testCut, testReso;
		testCut = 1.f; // WinMidi_GetCutoff();
		testReso = 0.1f;  // WinMidi_GetResonance();

		MOOG::SetCutoff(testCut*1000.f);
		MOOG::SetResonance(testReso*kPI);
		MOOG::SetDrive(1.f);
	}

	/*
		Note (voice) logic.
	*/

	// FIXME: first fit isn't fancy enough
	SFM_INLINE unsigned AllocNote(FM_Voice *voices)
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
	unsigned TriggerNote(unsigned midiIndex)
	{
		FM &state = s_shadowState;

//		std::lock_guard<std::shared_mutex> lock(s_stateMutex);

		const unsigned iVoice = AllocNote(s_shadowState.m_voices);
		if (-1 == iVoice)
			return -1;

		++state.m_active;

		FM_Voice &voice = state.m_voices[iVoice];

		// FIXME: adapt to patch when it's that time
		const float carrierFreq = g_midiToFreqLUT[midiIndex];
		voice.m_carrier.Initialize(kSine, kMaxVoiceAmplitude, carrierFreq);
		const float ratio = 4.f;
		const float CM = carrierFreq*ratio;
		voice.m_modulator.Initialize(0.f /* LFO? */, CM, 0.f); // These parameters mean a lot
		voice.m_ADSR.Start(s_sampleCount, 1.f /* FIXME */);

		voice.m_enabled = true;

		return iVoice;
	}

	void ReleaseNote(unsigned iVoice)
	{
		FM &state = s_shadowState;

//		std::lock_guard<std::shared_mutex> lock(s_stateMutex);
		FM_Voice &voice = state.m_voices[iVoice];
		voice.m_ADSR.Stop(s_sampleCount);

		// FIXME
		// const float angPitch = voice.m_carrier.m_angularPitch;
		const float vorticity = 0.75f;
		voice.m_vorticity.Initialize(s_sampleCount, vorticity*kPI, vorticity);
	}

	// FIXME: For now this is a hack that checks if enabled voices are fully released, and frees them,
	//        but I can see this function have more use later on (like, not at 07:10 AM)
	void UpdateNotes()
	{
//		std::lock_guard<std::shared_mutex> lock(s_stateMutex);

		FM &state = s_shadowState;

		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			FM_Voice &voice = state.m_voices[iVoice];
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
		FM_Voice *voices = state.m_voices;

		const unsigned numVoices = state.m_active;

		if (0 == numVoices)
		{
			// Silence, but still run (off) the filter
			memset(pDest, 0, numSamples*sizeof(float));
			MOOG::SetDrive(0.f);
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
					if (true == voice.m_enabled)
					{
						const float sample = voice.Sample();
						dry += sample;
					}
				}

				const float clipped = clampf(-1.f, 1.f, dry);
				pDest[iSample] = atanf(clipped);

				++s_sampleCount;
			}
		}

//		const float wetness = WinMidi_GetFilterMix();
		const float wetness = 1.f;
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
		FIXME: just write out one sample for 8 seconds, fix real output multi-platform (SDL)!
	*/
	
	const float duration = 2.f;
	const unsigned numSamples = kSampleRate*duration;

	float render[numSamples];
	
	UpdateFilterParameters();

	// Strike chord: C-E-G
	const unsigned index1 = TriggerNote(64);
	const unsigned index2 = TriggerNote(73);
	const unsigned index3 = TriggerNote(76);

	const unsigned release = s_shadowState.m_voices[index1].m_ADSR.m_release;

	unsigned writeIdx = 0;

	for (unsigned iSample = 0; iSample < numSamples-release; ++iSample)
	{		
		UpdateNotes();
		Render(render+writeIdx++, 1);
	}

	// Release chord.
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
//	float pitch = CalcSinLUTPitch(440.f);
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