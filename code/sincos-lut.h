
// cookiedough -- LUT sinus/cosinus (interpolated)

#pragma once

#include "Std3DMath-stripped/Math.h"

constexpr unsigned kCosTabSize = 2048;
extern "C" float g_cosLUT[kCosTabSize+1];

void CalculateCosLUT();

CKD_INLINE static float lutcosf(float angle) {
	angle = fabsf(angle); // prevent rounding asymmery for small negative angles
	angle *= (1.f/k2PI)*kCosTabSize;
	const int index = int(angle)&(kCosTabSize-1);
//	return g_cosLUT[index];
	return lerpf<float>(g_cosLUT[index], g_cosLUT[index+1], fracf(angle));
}

#if defined(ARRESTED_DEV_LEGACY)

// yes, I had *this* wrong earlier (you don't really notice that fast when working with abstract effects)
CKD_INLINE static float lutsinf(float angle) {
	return lutcosf(angle + kPI*0.5f);
}

#else

CKD_INLINE static float lutsinf(float angle) {
	return lutcosf(angle - kPI*0.5f);
}

#endif
