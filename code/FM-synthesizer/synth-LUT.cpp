
/*
	Syntherklaas FM -- Lookup tables.
*/

#include "synth-global.h"
// #include "synth-LUT.h"

namespace SFM
{
	// Taken from: http://noyzelab.blogspot.com/2016/04/farey-sequence-tables-for-fm-synthesis.html
	// Calculated using what's called a 'Farey Sequence'
	// Has 37 entries
	unsigned g_CM_table_15NF[][2] = 
	{
		{ 1, 1 }, { 1, 2 }, { 1, 3 }, { 1, 4 }, { 1, 5 }, { 1, 6 }, { 1, 7 }, { 1, 8 }, { 1, 9 }, { 1, 10 }, { 1, 11 }, { 1, 12 }, { 1, 13 }, { 1, 14 }, { 1, 15 },
		{ 2, 5 }, { 2, 7 }, { 2, 9 }, { 2, 11 }, { 2, 13 }, { 2, 15 },
		{ 3, 7 }, { 3, 8 }, { 3, 10 }, { 3, 11 }, { 3, 13 }, { 3, 14 },
		{ 4, 9 }, { 4, 11 }, { 4, 13 }, { 4, 15 },
		{ 5, 11 }, { 5, 12 }, { 5, 13 }, { 5, 14 },
		{ 6, 13 },
		{ 7, 15 }
	};

	alignas(16) float g_sinLUT[kOscPeriod];
	alignas(16) float g_noiseLUT[kOscPeriod];

	void CalculateLUTs()
	{
#if 1
		// Plain sinus
		float angle = 0.f;
		const float angleStep = k2PI/kOscPeriod;
		for (unsigned iStep = 0; iStep < kOscPeriod; ++iStep)
		{
			g_sinLUT[iStep] = sinf(angle);
			angle += angleStep;
		}
#endif

#if 0
		/* 
			Gordon-Smith oscillator
			Allows for frequency changes with minimal artifacts (first-order filter)
		*/

		const float frequency = 1.f;
		const float theta = k2PI*frequency / kOscPeriod;
		const float epsilon = 2.f * sinf(theta/2.f);
		
		float N, prevN = sinf(-1.f*theta);
		float Q, prevQ = cosf(-1.f*theta);

		for (unsigned iStep = 0; iStep < kOscPeriod; ++iStep)
		{
			Q = prevQ - epsilon*prevN;
			N = epsilon*Q + prevN;
			prevQ = Q;
			prevN = N;
			g_sinLUT[iStep] = ultra_tanhf(N);
		}
#endif

		// White noise (Mersenne-Twister)
		for (unsigned iStep = 0; iStep < kOscPeriod; ++iStep)
		{
			g_noiseLUT[iStep] = -1.f + 2.f*mt_randf();
		}
	}
}
