
/*
	Syntherklaas FM -- Global lookup tables.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// Supposedly Yamaha modulation ratios
	extern float g_modRatioLUT[];
	extern size_t g_numModRatios;

	// 9th-order normalized Farey sequence
	extern unsigned g_CM[][2];
	extern unsigned g_CM_size;

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
