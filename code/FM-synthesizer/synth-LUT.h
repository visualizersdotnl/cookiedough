
/*
	Syntherklaas FM -- Lookup table(s).
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	extern float g_sinLUT[kOscPeriod];
	extern float g_noiseLUT[kOscPeriod];

	void CalculateLUTs();

	/*
		LUT functions
	*/

	SFM_INLINE float LUTsample(float *LUT, float index)
	{
		// FIXME: use proper sampling
		return LUT[unsigned(index)&kOscTabAnd];
	}

	SFM_INLINE float lutsinf(float index) { return LUTsample(g_sinLUT, index); }
	SFM_INLINE float lutcosf(float index) { return lutsinf(index + kOscPeriod/4); }
	SFM_INLINE float lutnoisef(float index) { return LUTsample(g_noiseLUT, index); }
}
