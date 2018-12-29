
/*
	Syntherklaas FM -- Global lookup tables.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// See impl.
	extern float g_DX7_EG_to_OL[257];

	// Sine wave (Gordon-Smith)
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
