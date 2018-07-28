
// cookiedough -- LUT sinus/cosinus

#include "main.h"
// #include "sincos-lut.h"

float g_cosLUT[kCosTabSize];

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
