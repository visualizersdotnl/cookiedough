
/*
	Syntherklaas -- FM synthesizer prototype.
	Sinus lookup table.
*/

#ifndef _SFM_SINUS_LUT_H_
#define _SFM_SINUS_LUT_H_

#include "global.h"

namespace SFM
{
	const size_t kSinTabSize = 4096;
	constexpr size_t kSinTabAnd = kSinTabSize-1;

	extern float g_sinLUT[kSinTabSize];

	void CalculateSinLUT();

	SFM_INLINE float lutsinf(float index) { return g_sinLUT[unsigned(index)&kSinTabAnd]; }
};

#endif // _SFM_SINUS_LUT_H_
