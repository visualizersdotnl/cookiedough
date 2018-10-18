
/*
	Syntherklaas FM -- Lookup table(s).
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	const unsigned kSinTabSize = 4096;
	const unsigned kSinTabAnd = kSinTabSize-1;

	extern float g_sinLUT[kSinTabSize];

	void CalculateLUTs();

	SFM_INLINE float lutsinf(float index) 
	{
		// Nearest (FIXME)
		return g_sinLUT[unsigned(index)&kSinTabAnd]; 
	}
	
	SFM_INLINE float lutcosf(float index) { return lutsinf(index + kSinTabSize/4); }
}
