
/*
	Syntherklaas -- FM synthesizer prototype.
	Sinus lookup table.
*/

#include "main.h"
#include "synth-sin-lut.h"

alignas(16) float g_sinLUT[kSinTabSize];

void FM_CalculateSinLUT()
{
	float angle = 0.f;
	const float angleStep = k2PI/kSinTabSize;
	for (auto iStep = 0; iStep < kSinTabSize; ++iStep)
	{
		g_sinLUT[iStep] = sinf(angle);
		angle += angleStep;
	}
}
