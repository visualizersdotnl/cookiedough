
// cookiedough -- LUT sinus/cosinus

#include "main.h"
// #include "sincos-lut.h"

alignas(16) float g_cosLUT[kCosTabSize];

void CalculateCosLUT()
{
	float angle = 0.f;
	const float angleStep = k2PI/kCosTabSize;
	for (unsigned iStep = 0; iStep < kCosTabSize; ++iStep)
	{
		g_cosLUT[iStep] = cosf(angle);
		angle += angleStep;
	}
}
