
/*
	Syntherklaas FM -- Sinus lookup table.
*/

#include "synth-global.h"
// #include "synth-LUT.h"

namespace SFM
{
	alignas(16) float g_sinLUT[kOscPeriod];
	alignas(16) float g_noiseLUT[kOscPeriod];

	void CalculateLUTs()
	{
		// Sinus
		float angle = 0.f;
		const float angleStep = k2PI/kOscPeriod;
		for (unsigned iStep = 0; iStep < kOscPeriod; ++iStep)
		{
			g_sinLUT[iStep] = sinf(angle);
			angle += angleStep;
		}

		// Noise
		for (unsigned iStep = 0; iStep < kOscPeriod; ++iStep)
		{
			g_noiseLUT[iStep] = -1.f + 2.f*mt_randf();
		}
	}
}
