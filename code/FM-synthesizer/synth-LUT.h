
/*
	Syntherklaas FM -- Lookup tables.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// Generated Carrier:Modulation ratio table
	extern unsigned g_CM_table[][2];
	extern unsigned g_CM_table_size;

	// Sinus, noise [-1..1]
	const unsigned kOscLUTSize = 4096;

	extern float g_sinLUT[kOscLUTSize];
	extern float g_noiseLUT[kOscLUTSize];

	void CalculateLUTs();

	/*
		A full period is kOscLUTSize, so multiply before calling.
	*/

	SFM_INLINE float SampleLUT(const float *LUT, float index)
	{
		return LUT[unsigned(index) & kOscLUTSize-1];
	}

	SFM_INLINE float lutsinf(float index)   { return SampleLUT(g_sinLUT, index*kOscLUTSize);   }
	SFM_INLINE float lutcosf(float index)   { return lutsinf(index + 0.25f); }
	SFM_INLINE float lutnoisef(float index) { return SampleLUT(g_noiseLUT, index*kOscLUTSize); }
}
