
// cookiedough -- LUT sinus/cosinus

#include "main.h"
// #include "sincos-lut.h"

#ifdef _WIN32
	__declspec(align(16)) float g_cosLUT[kCosTabSize];
#else
	float g_cosLUT[kCosTabSize];
#endif

void CalculateCosLUT()
{
	float angle = 0.f;
	const float angleStep = (2*kPI)/kCosTabSize;
	for (auto iStep = 0; iStep < kCosTabSize; ++iStep)
	{
		g_cosLUT[iStep] = cosf(angle);
		angle += angleStep;
	}
}
