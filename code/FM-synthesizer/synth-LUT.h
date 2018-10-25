
/*
	Syntherklaas FM -- Lookup table(s).
*/

#pragma once

#include "synth-global.h"

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

	SFM_INLINE float LUTsample(const float *LUT, float index)
	{
		// Nearest neighbour; so far so good, but sooner or later: FIXME
		const unsigned from = unsigned(index)&kOscTabAnd;
		const float A = LUT[from];
		return A;
	}

	SFM_INLINE float lutsinf(float index)   { return LUTsample(g_sinLUT, index);    }
	SFM_INLINE float lutcosf(float index)   { return lutsinf(index + kOscPeriod/4); }
	SFM_INLINE float lutnoisef(float index) { return LUTsample(g_noiseLUT, index);  }
}
