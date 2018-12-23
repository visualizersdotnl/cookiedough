
/*
	Syntherklaas FM -- Global lookup tables.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// Hexter table(s) (see synth-Hexter-tables.h)
	extern float g_dx7_voice_lfo_frequency[128];
	extern float g_dx7_voice_eg_ol_to_mod_index_table[257];
	extern float g_dx7_voice_velocity_ol_adjustment[128];

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
