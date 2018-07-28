
// cookiedough -- LUT sinus/cosinus

/*
	FIXME: you'd get slightly better results by blending 2 samples)
*/

#pragma once

const size_t kCosTabSize = 1024;
constexpr size_t kCosTabAnd = kCosTabSize-1;
constexpr unsigned kTabSinPhase = 1024/4;

extern float g_cosLUT[kCosTabSize];

void CalculateCosLUT();

VIZ_INLINE float lutcosf(int angle)
{
	return g_cosLUT[angle&kCosTabAnd];
}

VIZ_INLINE float lutsinf(int angle)
{
	return lutcosf(kTabSinPhase+angle);
}
