
/*
	See: http://ccrma.stanford.edu/software/snd/snd/fm.html
	Important: Rocket not available (for now) in this mode, see main.cpp!
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

// TODO: add channels, can easily do 4 at a time with SSE and LUTs.
struct FM
{
	float time;

	// Single note
	float toneAmp;
	float carrierFreq;
	float modFreq;
	float modAmp;

} static s_FM;

static bool s_isReady = false;

void Syntherklaas_Render(uint32_t *pDest, float time, float delta)
{
	s_FM.time = time;

	// Define single note
	s_FM.carrierFreq = 440.f;
	s_FM.toneAmp = 1.f;
	s_FM.modFreq = powf(kPI, kPI);
	s_FM.modAmp = 1.f;

	if (false == s_isReady)
	{
		Audio_Start_Stream();
		s_isReady = true;
	}

	VIZ_ASSERT(true == Audio_Check_Stream());
}

DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser)
{
	const unsigned numSamples = length/sizeof(float);
	VIZ_ASSERT(length == numSamples*sizeof(float));

	// Not really necessary?
	const float time = s_FM.time;
	
	const float period = k2PI;
	const float amplitude = s_FM.toneAmp;
	const float modAmplitude = s_FM.modAmp;

	float carrierStep = period*(s_FM.carrierFreq/kSampleRate);
	float modStep = period*(s_FM.modFreq/kSampleRate);

	// These are static for now since I'm just adding by number of sample instead of adding the timer.
	static float carrierPhase = 0.f;
	static float modPhase = 0.f;

	float *pFloats = static_cast<float*>(pDest);
	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		// FM (Chowning)
		const float tone = amplitude*sinf(carrierPhase + modAmplitude*sinf(modPhase));

		*pFloats++ = tone;

		carrierPhase += carrierStep;
		modPhase += modStep;
	}

	return length;
}
