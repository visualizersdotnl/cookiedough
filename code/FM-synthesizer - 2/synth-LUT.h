
/*
	Syntherklaas FM -- Global lookup tables.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// Follows the Volca FMs coarse ratio table (an octave down, neutral and 30 semitones up)
	extern float g_ratioLUT[32];
	const size_t g_ratioLUTSize = sizeof(g_ratioLUT)/sizeof(float);

	// Sine wave
	extern float g_sinLUT[kOscLUTSize];

	void CalculateLUTs();

	//	A full period is kLUTSize, so multiply before calling
	SFM_INLINE float SampleLUT(const float *LUT, float index)
	{
		return LUT[unsigned(index) & kOscLUTAnd];
	}

	SFM_INLINE float lutsinf(float index) { return SampleLUT(g_sinLUT, index*kOscLUTSize);   }
	SFM_INLINE float lutcosf(float index) { return lutsinf(index + 0.25f); }
}
