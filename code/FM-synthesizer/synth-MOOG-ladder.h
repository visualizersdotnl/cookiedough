

/*
	Syntherklaas FM -- 4-pole filter like the MOOG ladder.

	Credits:
		- https://github.com/ddiakopoulos/MoogLadders/tree/master/src
		- The paper by S. D'Angelo & V. Välimäki (2013).

	Major win here:
		- Filter does not require oversampling \o/

	Added:
		- Taking edges off final sample using atanf().
		- Mixing with dry input.

	FIXME: more than enough to optimize here if need be (SIMD, maybe), but let us wait for a target platform.
*/

#ifndef _SFM_SYNTH_MOOG_LADDER_H_
#define _SFM_SYNTH_MOOG_LADDER_H_

#include "synth-global.h"
#include "fast-tanhf.h"

namespace SFM
{
	// Thermal voltage (26 milliwats at room temperature)
	// FIXME: offer different temperatures by a control next to cutoff and resonance?
	const float kVT = 0.312f;

	float g_cutGain;
	float g_resonance;
	float g_drive;

	// Filter deltas.
	float V[4]  = { 0.f };
	float dV[4] = { 0.f };
	float tV[4] = { 0.f };

	static void MOOG_Cutoff(float cutoff)
	{
		SFM_ASSERT(cutoff >= 0.f && cutoff <= 1000.f); 

		const float angular = (kPI*cutoff)/kSampleRate;
		const float gain = 4.f*kPI*kVT*cutoff*(1.f-angular)/(1.f+angular);
	
		g_cutGain = gain;
	}

	static void MOOG_Resonance(float resonance)
	{
		SFM_ASSERT(resonance >= 0.f && resonance <= 4.f);
		g_resonance = resonance;
	}

	static void MOOG_Drive(float drive)
	{
		g_drive = drive;
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

	SFM_INLINE void MOOG_Reset()
	{
		MOOG_Reset_Feedback();
		MOOG_Reset_Parameters();
	}

	static void MOOG_Ladder(float *pDest, unsigned numSamples, float wetness)
	{
		float dV0, dV1, dV2, dV3;

		const float &resonance = g_resonance;
		const float &cutGain = g_cutGain;
		const float &drive = g_drive;

		// FIXME: find a more elegant, faster way to express this (SIMD, more LUTs)
		for (unsigned iSample = 0; iSample < numSamples; ++iSample)
		{
			// Fetch dry sample, multiply by drive since I use it to mix voices
			const float dry = drive*pDest[iSample];

			dV0 = -cutGain * (fast_tanhf((dry + resonance*V[3]) / (2.f*kVT)) + tV[0]);
			V[0] += (dV0 + dV[0]) / (2.f*kSampleRate);
			dV[0] = dV0;
			tV[0] = fast_tanhf(V[0]/(2.f*kVT));
			
			dV1 = cutGain * (tV[0] - tV[1]);
			V[1] += (dV1 + dV[1]) / (2.f*kSampleRate);
			dV[1] = dV1;
			tV[1] = fast_tanhf(V[1]/(2.f*kVT));
			
			dV2 = cutGain * (tV[1] - tV[2]);
			V[2] += (dV2 + dV[2]) / (2.f*kSampleRate);
			dV[2] = dV2;
			tV[2] = fast_tanhf(V[2]/(2.f*kVT));
			
			dV3 = cutGain * (tV[2] - tV[3]);
			V[3] += (dV3 + dV[3]) / (2.f*kSampleRate);
			dV[3] = dV3;
			tV[3] = fast_tanhf(V[3]/(2.f*kVT));
			
			// Sigmoid curve blend between dry and wet, then take the edges off to prevent clipping
			pDest[iSample] = atanf(smoothstepf(dry, V[3], wetness));
		}
	}
}

#endif // _SFM_SYNTH_MOOG_LADDER_H_
