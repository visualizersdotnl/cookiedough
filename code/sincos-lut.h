
// cookiedough -- LUT sinus/cosinus

/*
	FIXME:
	- you'd get slightly better results by blending 2 samples
*/

#pragma once

const size_t kCosTabSize = 2048;
constexpr size_t kCosTabAnd = kCosTabSize-1;
constexpr unsigned kTabSinPhase = kCosTabSize/4;
extern float g_cosLUT[kCosTabSize];

void CalculateCosLUT();

VIZ_INLINE float lutcosf(int angleIndex) { return g_cosLUT[angleIndex&kCosTabAnd];  }
VIZ_INLINE float lutsinf(int angleIndex) { return lutcosf((int) kTabSinPhase+angleIndex); }

// always try to use this whenever using the angle more than once
VIZ_INLINE int tocosindex(float angle)
{
	angle *= 1.f/k2PI;
	angle *= kCosTabSize;
	return (int) angle;
}

VIZ_INLINE float lutcosf(float angle) { return lutcosf(tocosindex(angle)); }
VIZ_INLINE float lutsinf(float angle) { return lutsinf(tocosindex(angle)); }
