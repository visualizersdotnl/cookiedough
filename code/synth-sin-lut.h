
/*
	Syntherklaas -- FM synthesizer prototype.
	Sinus lookup table.
*/

#pragma once

const size_t kSinTabSize = 4096;
constexpr size_t kSinTabAnd = kSinTabSize-1;
extern float g_sinLUT[kSinTabSize];

void FM_CalculateSinLUT();

VIZ_INLINE float FM_lutsinf(float index) { return g_sinLUT[unsigned(index)&kSinTabAnd]; }
