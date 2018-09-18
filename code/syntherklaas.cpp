
/*
	See: http://ccrma.stanford.edu/software/snd/snd/fm.html
	Important: Rocket not available (for now) in this mode, see main.cpp!

	Missing:
	- ADSR
	- FIR
	- Reverb
	- More waveforms
	- Multiple notes
	- Presets
	- MIDI support
*/

#include "main.h"
#include "syntherklaas.h"
// #include "rocket.h"

const float kSampleRate = 44100.f;

bool Syntherklaas_Create()
{
	return true;
}

void Syntherklaas_Destroy()
{
}

enum WaveForm
{
	kSine,
	kSaw
};

struct FM_Note
{
	WaveForm form;

	float amplitude;
	float carrierFreq;
	float modulationFreq;
	float modulationIndex;

	// To be initialized and updated during rendering.
	float carrierPhase;
	float modulationPhase;
};

struct FM
{
	// FIXME: unused!
	float time;

	// Single note/instrument.
	FM_Note note;

} static s_FM;

static bool s_isReady = false;

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	s_FM.time = time;

	if (false == s_isReady)
	{
		// Define single note
		s_FM.note.form = kSaw;
		s_FM.note.carrierFreq = 440.f;
		s_FM.note.amplitude = 1.f;
		s_FM.note.modulationFreq = 4.f;
		s_FM.note.modulationIndex = 2.f;

		// Reset phases
		s_FM.note.carrierPhase = 0.f;
		s_FM.note.modulationPhase = 0.f;

		Audio_Start_Stream();

		s_isReady = true;
	}

	// Check for stream starvation
	VIZ_ASSERT(true == Audio_Check_Stream());
}

DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser)
{
	const unsigned numSamples = length/sizeof(float);
	VIZ_ASSERT(length == numSamples*sizeof(float));

	// Not really necessary?
	const float time = s_FM.time;
	
	const float period = k2PI;
	const float amplitude = s_FM.note.amplitude;
	const float modIndex = s_FM.note.modulationIndex;

	float carrierStep = period*s_FM.note.carrierFreq/kSampleRate;
	float modStep = period*s_FM.note.modulationFreq/kSampleRate;

	float *pFloats = static_cast<float*>(pDest);
	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		// FM (Chowning)
		const float modulation = modIndex*sinf(s_FM.note.modulationPhase);

		float waveform;
		switch (s_FM.note.form)
		{
			default:
			case kSine:
				waveform = sinf(s_FM.note.carrierPhase + modulation);
				break;

			case kSaw:
				waveform = -1.f + fmodf(s_FM.note.carrierPhase + modulation, period);
				break;
		}

		const float tone = amplitude*waveform;

		*pFloats++ = tone;

		s_FM.note.carrierPhase += carrierStep;
		s_FM.note.modulationPhase += modStep;
	}

	return length;
}
