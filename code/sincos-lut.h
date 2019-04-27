
// cookiedough -- LUT sinus/cosinus

#pragma once

const size_t kCosTabSize = 4096*2;
constexpr size_t kCosTabAnd = kCosTabSize-1;
constexpr unsigned kCosTabSinPhase = kCosTabSize/4;
extern float g_cosLUT[kCosTabSize];

void CalculateCosLUT();

VIZ_INLINE float lutcosf(int index) { return g_cosLUT[index&kCosTabAnd];  }
VIZ_INLINE float lutsinf(int index) { return lutcosf((int) kCosTabSinPhase+index); }

// always try to use this whenever using the angle more than once
VIZ_INLINE int tocosindex(float angle)
{
	angle *= 1.f/k2PI;
	angle *= kCosTabSize;
	return (int) angle;
}

VIZ_INLINE float lutcosf(float angle) {
	return lutcosf(tocosindex(angle)); 
}

VIZ_INLINE float lutsinf(float angle) { return lutsinf(tocosindex(angle)); }
