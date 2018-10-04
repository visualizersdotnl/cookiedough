
/*
	Syntherklaas FM -- Sinus lookup table.
*/

#include "synth-global.h"
#include "sinus-LUT.h"

namespace SFM
{
	alignas(16) float g_sinLUT[kSinTabSize];

	void CalculateSinLUT()
	{
		float angle = 0.f;
		const float angleStep = k2PI/kSinTabSize;
		for (auto iStep = 0; iStep < kSinTabSize; ++iStep)
		{
			g_sinLUT[iStep] = sinf(angle);
			angle += angleStep;
		}
	}
}
