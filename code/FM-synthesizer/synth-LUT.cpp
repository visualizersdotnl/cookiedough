
/*
	Syntherklaas FM -- Sinus lookup table.
*/

#include "synth-global.h"
// #include "synth-LUT.h"

namespace SFM
{
	alignas(16) float g_sinLUT[kSinTabSize];

	void CalculateLUTs()
	{
		float angle = 0.f;
		const float angleStep = k2PI/kSinTabSize;
		for (unsigned iStep = 0; iStep < kSinTabSize; ++iStep)
		{
			g_sinLUT[iStep] = sinf(angle);
			angle += angleStep;
		}
	}
}
