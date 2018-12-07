
/*
	Syntherklaas FM -- Global lookup tables.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// Hexter table(s)
	extern float g_dx7_voice_lfo_frequency[128];

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
