
/*
	Syntherklaas FM presents 'FM. BISON'
	(C) syntherklaas.org, a subsidiary of visualizers.nl

	This is intended to be a powerful yet relatively simple FM synthesizer core.

	I have to thank the following people for helping me with their knowledge, ideas and experience:
		- Ronny Pries
		- Tammo Hinrichs
		- Alex Bartholomeus
		- Pieter v/d Meer
		- Thorsten Ørts
		- Stijn Haring-Kuipers
		- Dennis de Bruijn
		- Zden Hlinka

	It's intended to be portable to embedded platforms in plain C (which it isn't now but close enough), 
	and in part supplemented by hardware components if that so happens to be a good idea.

	To do:
	- Fix: use a shadow state that can be updated by MIDI at will and is then copied on render
	- Fix: any NAN-bugs left, why?
	- Implement ADSR on voices!
	- Optimization, FIXMEs, interpolation.
	- Smooth out MIDI controls using Maarten van Strien's trick.
	- Normalize volumes?
	- Zie notebook.
*/

// MSVC :(
#define _CRT_SECURE_NO_WARNINGS

// FIXME: easily interchangeable
#include <mutex>

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
		FM modulator.
	*/

	void FM_Modulator::Initialize(float index, float frequency)
	{
		m_index = index;
		m_pitch = CalcSinPitch(frequency);
		m_sample = 0;
	}

	float FM_Modulator::Sample(const float *pEnv)
	{
		float phase = m_sample*m_pitch;

		float envelope = 1.f;
		if (pEnv != nullptr)
		{
			const unsigned index = m_sample & (kPeriodLength-1);
			envelope = pEnv[index];
		}

		++m_sample;

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
		m_sample = 0;
		m_numHarmonics = GetCarrierHarmonics(frequency);
	}

	// FIXME: kill switch!
	float FM_Carrier::Sample(float modulation)
	{
		const float phase = m_sample*m_pitch;
		++m_sample;

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
		Global state.
	*/

	static FM s_FM;

	/*
		MIDI-driven MOOG ladder filter setup.
	*/

	static void SetMOOG()
	{
		float testCut, testReso;
		testCut = WinMidi_GetCutoff();
		testReso = WinMidi_GetResonance();
		MOOG_Cutoff(testCut*1000.f);
		MOOG_Resonance(testReso*4.f);
	}

	/*
		Note (voice) logic.
	*/

	SFM_INLINE unsigned AllocNote()
	{
		FM_Voice *voices = s_FM.voices;

		// First fit (FIXME)
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
		const unsigned iVoice = AllocNote();
		if (-1 != iVoice)
		{
			FM_Voice &voice = s_FM.voices[iVoice];

			// FIXME: patch!
			const float carrierFreq = g_midiToFreqLUT[midiIndex];
			voice.carrier.Initialize(kDirtySaw, 1.f, carrierFreq);
			const float ratio = 5.f/3.f;
			const float CM = carrierFreq*ratio;
			voice.modulator.Initialize(1.f /* LFO! */, CM);

			voice.enabled = true;
		}

		return iVoice;
	}

	void ReleaseNote(unsigned index)
	{
		FM_Voice &voice = s_FM.voices[index];

		voice.enabled = false;
	}

	/*
		Render function.
		FIXME: crude impl., optimize.
	*/

	static void Render(float *pDest, unsigned numSamples)
	{
		unsigned numVoices = 0;
		for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			if (true == s_FM.voices[iVoice].enabled) ++numVoices;

		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			float dry = 0.f;
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				FM_Voice &voice = s_FM.voices[iVoice];
				if (true == voice.enabled)
				{
					dry += voice.Sample();
				}
			}

			pDest[iSample] = dry;
		}

		const float wetness = WinMidi_GetFilterMix();

		// Only filter if needed
		if (0 != numVoices)
		{
			MOOG_Drive(1.f/numVoices);
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
		s_FM.Reset();
		SetTestVoice(s_FM.voices[0]);
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

	// Reset FM state & MOOG ladder
	s_FM.Reset();
	MOOG_Reset();

	const auto numDevs = WinMidi_GetNumDevices();
	const bool midiIn = WinMidi_Start(0);

	return true == midiIn;
}

void Syntherklaas_Destroy()
{
	WinMidi_Stop();
}

/*
	Mutex.
*/

static std::mutex s_mutex;

/*
	Render function for Kurt Bevacqua codebase.
*/

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	// Update MOOG filter parameters
	SetMOOG();

	if (false == s_isReady)
	{
		// Start blasting!
		Audio_Start_Stream();
		s_isReady = true;
	}

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

	const unsigned numSamples = std::min<unsigned>(numSamplesReq, kRingBufferSize);
	Render(static_cast<float*>(pDest), numSamples);

	return numSamples*sizeof(float);
}
