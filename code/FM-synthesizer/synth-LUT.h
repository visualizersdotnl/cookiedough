
/*
	Syntherklaas FM -- Lookup tables.
*/

#pragma once

#include "synth-global.h"
#include "synth-util.h"

namespace SFM
{
	extern float g_sinLUT[kOscPeriod];
	extern float g_noiseLUT[kOscPeriod];

	// Carrier:Modulation ratio table (37 entries)
	const unsigned kCarrierModTabSize = 37;
	extern unsigned g_CM_table_15NF[][2];

	void CalculateLUTs();

	/*
		LUT functions
	*/

	// FIXME: depending on a few factors interpolation might be unnecessary
	SFM_INLINE float SampleLUT(const float *LUT, float index)
	{
//		unsigned from = unsigned(index);
//		const float A = LUT[from&kOscTabAnd];
//		return A;

		unsigned from = unsigned(index);
		unsigned to = from+1;
		const float delta = fracf(index);
		const float A = LUT[from&kOscTabAnd];
		const float B = LUT[to&kOscTabAnd];
		const float sample = lerpf<float>(A, B, delta);
		return sample;
	}

	SFM_INLINE float lutsinf(float index)   { return SampleLUT(g_sinLUT, index);      }
	SFM_INLINE float lutcosf(float index)   { return lutsinf(index + kOscPeriod/4.f); }
	SFM_INLINE float lutnoisef(float index) { return SampleLUT(g_noiseLUT, index);    }
}
