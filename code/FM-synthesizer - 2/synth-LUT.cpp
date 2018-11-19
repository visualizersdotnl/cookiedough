
/*
	Syntherklaas FM -- Global lookup tables.
*/

#include "synth-global.h"
// #include "synth-LUT.h"

namespace SFM
{
	// Source: http://noyzelab.blogspot.com/2016/04/farey-sequence-tables-for-fm-synthesis.html
	unsigned g_CM[][2] = {
		{1, 1},
		{1, 2},
		{1, 3},
		{1, 4},
		{1, 5}, {2, 5},
		{1, 6},
		{1, 7}, {2, 7}, {3, 7},
		{1, 8}, {3, 8},
		{1, 9}, {2, 9}, {4, 9}
	};

	unsigned g_CM_size = sizeof(g_CM)/(2*sizeof(unsigned));

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
