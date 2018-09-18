
/*
	See: http://ccrma.stanford.edu/software/snd/snd/fm.html
	Important: Rocket not available (for now) in this mode, see main.cpp!

	Missing:
	- implement: chorus!
	- ADSR
	- Reverb
	- Flanger
	- More waveforms? Pulse + Triangle!
	- MIDI support
	- Multiple notes
	- FIR (oversample + Hamming/TOP6?)

	- Presets
*/

#include "main.h"
#include "syntherklaas.h"
// #include "rocket.h"

const float kSampleRate = 44100.f;
const unsigned kDelayLineLength = 32;

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
	// kTriangle
	// kPulse
};

// 4 basic oscillators:
VIZ_INLINE float oscSine(float beta) { return sinf(beta); }
VIZ_INLINE float oscSaw(float beta, float period) { return -1.f + fmodf(beta, period); }
// VIZ_INLINE float oscTriangle(...)
// VIZ_INLINE float oscPulse(...)

class FM_Note
{
public:
	FM_Note() :
		carrierPhase(0.f), modulationPhase(0.f) {}

	WaveForm form;

	float amplitude;
	float carrierFreq;
	float modulationFreq;
	float modulationIndex;

	// FIXME: best to just have 1 frequency and check it
	// FIXME: LFO now just uses oscSine()
	bool chorus;
	float chorusFreq;

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
		s_FM.note.modulationFreq = 2.314f;
		s_FM.note.modulationIndex = 314.f;

		// Turn on chorus
		s_FM.note.chorus = true;
		s_FM.note.chorusFreq = kPI;

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

	// FIXME: oversample and FIR!
	float *pFloats = static_cast<float*>(pDest);
	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		// FM (Chowning)
		const float modulation = modIndex*sinf(s_FM.note.modulationPhase);

		float waveform = 0.f;
		switch (s_FM.note.form)
		{
			default:
			case kSine:
				waveform += oscSine(s_FM.note.carrierPhase + modulation);
				break;

			case kSaw:
				waveform += oscSaw(s_FM.note.carrierPhase + modulation, period);
				break;
		}

		const float tone = amplitude*waveform;

		*pFloats++ = tone;

		s_FM.note.carrierPhase += carrierStep;
		s_FM.note.modulationPhase += modStep;
	}

	return length;
}
