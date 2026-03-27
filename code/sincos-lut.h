
// cookiedough -- LUT sinus/cosinus (interpolated)

// FIXME: temporarily reverted, doesn't play nice with Arrested Dev. (yet), check 'Love Prism' part (once that one animates properly, it does)

#pragma once

constexpr unsigned kCosTabSize = 4096;
constexpr unsigned kCosTabAnd = kCosTabSize-1;
constexpr int kCosTabSinPhase = kCosTabSize/4;

extern "C" float g_cosLUT[kCosTabSize+1];

void CalculateCosLUT();

CKD_INLINE static int64_t tocosindex(float angle) {
	angle *= (1.f/k2PI)*kCosTabSize;
//	angle *= 65536.f; // 16-bit fraction
	return int64_t(angle); // FIXME: possible truncation issue for small negative values
}

CKD_INLINE static float lutcosf(int64_t packedIndex) 
{ 
	return g_cosLUT[packedIndex&kCosTabAnd];
//	const int index = (packedIndex>>16)&kCosTabAnd;
//	return lerpf<float>(g_cosLUT[index], g_cosLUT[index+1], float(packedIndex&65535)*(1.f/65536.f));
}

CKD_INLINE static float lutsinf(int64_t packedIndex) 
{ 
	return lutcosf(kCosTabSinPhase+packedIndex);
//	return lutcosf(int64_t((kCosTabSinPhase<<16)+packedIndex)); 
}

CKD_INLINE static float lutcosf(float angle) { return lutcosf(tocosindex(angle)); }
CKD_INLINE static float lutsinf(float angle) { return lutsinf(tocosindex(angle)); }
