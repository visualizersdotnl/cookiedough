
/*
	Syntherklaas FM presents 'FM. BISON'
	(C) syntherklaas.org, a subsidiary of visualizers.nl

	Uses a little hack in the BASS layer of this codebase (audio.cpp) to output a 44.1kHz monaural stream.
*/

// Oh MSVC and your well-intentioned madness!
#define _CRT_SECURE_NO_WARNINGS

// FIXME: easily interchangeable with POSIX-variants or straight assembler
#include <mutex>
#include <atomic>

#include "FM_BISON.h"
#include "synth-MOOG-ladder.h"
#include "synth-LFOs.h"
#include "synth-midi.h"
#include "synth-oscillators.h"
#include "synth-state.h"
#include "synth-ringbuffer.h"

// Win32 MIDI input (specialized for the Oxygen 49)
#include "windows-midi-in.h"

namespace SFM
{
	/*
		Ring buffer.
	*/

	static FIFO s_ringBuffer;

	/*
		Global sample count.

		Each timed object stores an offset when it is activated and uses this global acount to calculate the delta, which is relatively cheap
		and keeps the concept time largely out of the floating point realm.

		FIXME: test/account for wrapping!
	*/

	static std::atomic<unsigned> s_sampleCount = 0;

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
			- Pulling state (e.g. SetMOOG()) is fine, for now.

		This creates a tiny additional lag between input and output, but an earlier commercial product has shown this
		to be negligible so long as the update buffer's size isn't too large.
	*/

	static std::mutex s_stateMutex;
	static FM s_shadowState;
	static FM s_renderState;

	/*
		MIDI-driven MOOG ladder filter setup.

		The MOOG state is not included in the global state for now as it pulls the values rather than pushing them.
	*/

	static void SetMOOG()
	{
		float testCut, testReso;
		testCut = WinMidi_GetCutoff();
		testReso = WinMidi_GetResonance();
		MOOG_SetCutoff(testCut*1000.f);
		MOOG_SetResonance(testReso*4.f);
		MOOG_SetDrive(1.f);
	}

	/*
		Note (voice) logic.
	*/

	// First fit (FIXME: check lifetime, volume, pitch, just try different heuristics)
	SFM_INLINE unsigned AllocNote(FM_Voice *voices)
	{
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
		{
			if (false == voices[iVoice].enabled)
			{
				return iVoice;
			}
		}

		return -1;
	}

	// Returns voice index, if available
	unsigned TriggerNote(unsigned midiIndex)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		const unsigned iVoice = AllocNote(s_shadowState.voices);
		if (-1 == iVoice)
			return -1;

		FM_Voice &voice = state.voices[iVoice];

		// FIXME: adapt to patch when it's that time
		const float carrierFreq = g_midiToFreqLUT[midiIndex];
		voice.carrier.Initialize(kDirtySaw, 1.f, carrierFreq);
		const float ratio = 5.f/3.f;
		const float CM = carrierFreq*ratio;
		voice.modulator.Initialize(kPI/CM /* LFO! */, CM);

		voice.enabled = true;

		return iVoice;
	}

	void ReleaseNote(unsigned index)
	{
		std::lock_guard<std::mutex> lock(s_stateMutex);
		FM &state = s_shadowState;

		FM_Voice &voice = state.voices[index];

		voice.enabled = false;
	}

	/*
		Render function.
		FIXME: crude impl., optimize.
	*/

	static void Render(float *pDest, unsigned numSamples)
	{
		FM &state = s_renderState;
		FM_Voice *voices = state.voices;

		// FIXME: I can safely count these elsewhere now that I've got a lock
		unsigned numVoices = 0;
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			if (true == voices[iVoice].enabled) ++numVoices;

		// Render dry samples
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			float dry = 0.f;
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				FM_Voice &voice = voices[iVoice];
				if (true == voice.enabled)
				{
					dry += voice.Sample();
				}
			}

			pDest[iSample] = dry;
			++s_sampleCount;
		}

		// Only filter if needed
		const float wetness = WinMidi_GetFilterMix();
		if (0 != numVoices)
		{
			MOOG_SetDrive(1.f/numVoices);
			MOOG_Ladder(pDest, numSamples, wetness);
		}
	}

	// FIXME: hack for now to initialize an indefinite voice and start the stream.
	static bool s_isReady = false;

	/*
		Simple RAW waveform writer.
		FIXME?
	*/

	static void SetTestVoice(FM_Voice &voice)
	{
		// Define single voice (indefinite)
		const float carrierFreq = 440.f;
		voice.carrier.Initialize(kDirtySaw, 1.f, carrierFreq);
		const float ratio = 5.f/3.f;
		const float CM = carrierFreq*ratio;
		voice.modulator.Initialize(1.f /* LFO! */, CM);

		voice.enabled = true;
	}

	static void WriteToFile(unsigned seconds)
	{
		// Test voice, default MOOG ladder
		s_renderState.Reset();
		SetTestVoice(s_renderState.voices[0]);
		MOOG_Reset();

		// Render a fixed amount of seconds to a buffer
		const unsigned numSamples = seconds*kSampleRate;
		float *buffer = new float[numSamples];

		const float timeStep = 1.f/kSampleRate;
		float time = 0.f;
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			Render(buffer+iSample, 1);
			time += timeStep;
		}

		// And flush (no error checking)
		FILE* file;
		file = fopen("fm_test.raw", "wb");
		fwrite(buffer, sizeof(float), numSamples, file);
		fclose(file);

		delete buffer;
	}

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

	// Reset shadow FM state & MOOG ladder
	s_shadowState.Reset();
	MOOG_Reset();

	// Reset sample count
	s_sampleCount = 0;

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
	// Update MOOG filter parameters
	SetMOOG();

	if (false == s_isReady)
	{
//		WriteToFile(4.f);

		// Start blasting!
		Audio_Start_Stream();
		s_isReady = true;
	}

	// FIXME: fill ring buffer here!

	// Check for stream starvation
	SFM_ASSERT(true == Audio_Check_Stream());
}

/*
	Stream callback (FIXME)
*/

DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser)
{
	const unsigned numSamplesReq = length/sizeof(float);
	SFM_ASSERT(length == numSamplesReq*sizeof(float));

	// Lock shadow state and copy it to render state (introduces a tiny bit of latency).
	s_stateMutex.lock();
	s_renderState = s_shadowState;
	s_stateMutex.unlock();

	const unsigned numSamples = std::min<unsigned>(numSamplesReq, kRingBufferSize);
	Render(static_cast<float*>(pDest), numSamples);

	return numSamples*sizeof(float);
}
