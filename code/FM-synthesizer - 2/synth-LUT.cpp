
/*
	Syntherklaas FM -- Global lookup tables.
*/

#include "synth-global.h"
// #include "synth-LUT.h"

namespace SFM
{
	// Source: https://www.gearslutz.com/board/electronic-music-instruments-and-electronic-music-production/1166873-fm-operator-frequency-ratios.html
	float g_modRatioLUT[] {
		0.50f, 0.71f, 0.78f, 0.87f, 1.00f, 1.41f, 1.57f, 1.73f, 
		2.00f, 2.82f, 3.00f, 3.14f, 3.46f, 4.00f, 4.24f, 4.71f, 
		5.00f, 5.19f, 5.65f, 6.00f, 6.28f, 6.92f, 7.00f, 7.07f, 
		7.85f, 8.00f, 8.48f, 8.65f, 9.00f, 9.42f, 9.89f, 10.00f,
		10.38f, 10.99f, 11.00f, 11.30f, 12.00f, 12.11f, 12.56f, 
		12.72f, 13.00f, 13.84f, 14.00f, 14.10f, 14.13f, 15.00f, 
		15.55f, 15.57f, 15.70f, 16.96f, 17.27f, 17.30f, 18.37f, 
		18.84f, 19.03f, 19.78f, 20.41f, 20.76f, 21.20f, 21.98f, 
		22.49, 23.55f, 24.22f, 25.95f
	};

	size_t g_numModRatios = sizeof(g_modRatioLUT)/sizeof(float);

	// Sinus
	alignas(16) float g_sinLUT[kOscLUTSize];

	/*
		Depending on target platform/hardware I may want to dump this to disk/ROM.
	*/

	void CalculateLUTs()
	{
		/* 
			Gordon-Smith oscillator (sine wave generator)
		*/

		const float frequency = 1.f;
		const float theta = k2PI*frequency/kOscLUTSize;
		const float epsilon = 2.f*sinf(theta/2.f);
		
		float N, prevN = sinf(-1.f*theta);
		float Q, prevQ = cosf(-1.f*theta);

		for (unsigned iStep = 0; iStep < kOscLUTSize; ++iStep)
		{
			Q = prevQ - epsilon*prevN;
			N = epsilon*Q + prevN;
			prevQ = Q;
			prevN = N;
			g_sinLUT[iStep] = Clamp(N);
		}
	}
}

