
/*
	Syntherklaas FM -- Global lookup tables.
*/

#pragma once

#include "synth-global.h"
#include "synth-util.h"

namespace SFM
{
	// See impl.
	extern float g_DX7_LFO_speed[128];

	// Sine wave (Gordon-Smith)
	extern float g_sinLUT[kOscLUTSize];

	void CalculateLUTs();

	// A full period is kLUTSize, so multiply before calling
	SFM_INLINE float SampleLUT(const float *LUT, float index)
	{
		const float fraction = fracf(index);
		const unsigned from = unsigned(index);
		const unsigned to = from+1;
		const float A = LUT[from&kOscLUTAnd];
		const float B = LUT[to&kOscLUTAnd];
		const float value = lerpf<float>(A, B, fraction);
		return value;
	}

	SFM_INLINE float lutsinf(float index) { return SampleLUT(g_sinLUT, index*kOscLUTSize); }
	SFM_INLINE float lutcosf(float index) { return lutsinf(index + 0.25f); }
}
