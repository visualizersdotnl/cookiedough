
/*
	Syntherklaas FM -- Global lookup tables.
*/

#include "synth-global.h"
// #include "synth-LUT.h"

// Include tables I borrowed from Hexter
#include "synth-Hexter-tables.h"

namespace SFM
{
	// Sinus
	alignas(16) float g_sinLUT[kOscLUTSize];

	/*
		Depending on target platform/hardware I may want to dump this to disk/ROM.
		Also, in reality, I only neet 1/4th of it.
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

