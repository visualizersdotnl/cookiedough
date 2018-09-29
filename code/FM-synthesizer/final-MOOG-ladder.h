

/*
	Syntherklaas FM
	(C) syntherklaas.org, a subsidiary of visualizers.nl

	Post-filter like the MOOG ladder.

	Sources:
		- https://github.com/ddiakopoulos/MoogLadders/tree/master/src
		- The paper by S. D'Angelo & V. Välimäki (2013).

	Major win here:
		- Filter does not require oversampling!

	FIXME: clamp instead of assert, or not at all.
	FIXME: more than enough to optimize here if need be, but let us wait for a target platform.
*/

#ifndef _SFM_MOOG_LADDER_H_
#define _SFM_MOOG_LADDER_H_

#include "global.h"
#include "fast-tanhf.h"

namespace SFM
{

	// Thermal voltage (26 milliwats at room temperature)
	const float kVT = 0.312f;
	const float kThermalMul = 1.f/(2.f*kVT);

	float g_cutGain;
	float g_resonance;
	float g_drive;

	// Filter deltas.
	float V[4]  = { 0.f };
	float dV[4] = { 0.f };
	float tV[4] = { 0.f };

	static void MOOG_Cutoff(float cutoff)
	{
		// It seems reasonable that you can cutoff possibly inaudible frequency or harmonics.
		SFM_ASSERT(cutoff >= 0.f); 

		const float periodCut = (kPI*cutoff)/kSampleRate;
		const float gain = 4.f*kPI*kVT*cutoff*(1.f-periodCut)/(1.f+periodCut);
	
		g_cutGain = gain;
	}

	static void MOOG_Resonance(float resonance)
	{
		g_resonance = resonance;
	}

	static void MOOG_Reset_Parameters()
	{
		MOOG_Cutoff(1000.f);
		MOOG_Resonance(0.1f);
		g_drive = 1.f;
	}

	static void MOOG_Reset_Feedback()
	{
		for (unsigned iPole = 0; iPole < 4; ++iPole)
		{
			V[iPole]  = 0.f;
			dV[iPole] = 0.f;
			tV[iPole] = 0.f;
		}
	}

	static void MOOG_Ladder(float *pDest, unsigned numSamples)
	{
		float dV0, dV1, dV2, dV3;

		const float &resonance = g_resonance;
		const float &cutGain = g_cutGain;
		const float &drive = g_drive;

		// FIXME: find a more elegant way to write this down (and compare the two to assert correctness)
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			dV0 = -cutGain * (fast_tanhf((drive*pDest[iSample] + resonance*V[3]) * kThermalMul) + tV[0]);
			V[0] += (dV0 + dV[0]) / (2.f * kSampleRate);
			dV[0] = dV0;
			tV[0] = fast_tanhf(V[0]*kThermalMul);
			
			dV1 = cutGain * (tV[0] - tV[1]);
			V[1] += (dV1 + dV[1]) / (2.f * kSampleRate);
			dV[1] = dV1;
			tV[1] = fast_tanhf(V[1]*kThermalMul);
			
			dV2 = cutGain * (tV[1] - tV[2]);
			V[2] += (dV2 + dV[2]) / (2.f * kSampleRate);
			dV[2] = dV2;
			tV[2] = fast_tanhf(V[2]*kThermalMul);
			
			dV3 = cutGain * (tV[2] - tV[3]);
			V[3] += (dV3 + dV[3]) / (2.f * kSampleRate);
			dV[3] = dV3;
			tV[3] = fast_tanhf(V[3]*kThermalMul);

			// I'd like to be sure the fitler doesn't go out of bounds.
			const float sample = V[3];
			SFM_ASSERT(sample >= -1.f && sample <= 1.f);
			
			pDest[iSample] = V[3];
		}
	}
}

#endif // _SFM_MOOG_LADDER_H_
