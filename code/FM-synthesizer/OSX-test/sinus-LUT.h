
/*
	Syntherklaas FM -- Sinus lookup table.
*/

#ifndef _SFM_SINUS_LUT_H_
#define _SFM_SINUS_LUT_H_

#include "synth-global.h"

namespace SFM
{
	const size_t kSinTabSize = 4096;
	constexpr size_t kSinTabAnd = kSinTabSize-1;

	extern float g_sinLUT[kSinTabSize];

	void CalculateSinLUT();

	SFM_INLINE float lutsinf(float index) { return g_sinLUT[unsigned(index)&kSinTabAnd]; }
	SFM_INLINE float lutcosf(float index) { return lutsinf(index + kSinTabSize/4); }
}

#endif // _SFM_SINUS_LUT_H_
