
/*
	Syntherklaas -- FM synthesizer prototype.

	An implementation akin to the Moog ladder filter.
	To be used as a global final step.

	Based on a paper by S. DAngelo and V. Välimäki.
	Does not require oversampling.

	FIXME: clamp instead of assert?
	FIXME: more than enough to optimize here if need be, but let us wait for a target platform.
*/

#pragma once

// Thermal voltage (26 milliwats at room temperature)
const float kVT = 0.312f;
const float kThermalMul = 1.f/(2.f*kVT);

const unsigned kTaps = 4;

struct MoogLadder
{
	float cutoff;
	float resonance;

	float V[kTaps];
	float dV[kTaps];
	float tV[kTaps];

	float periodCut, cutGain;
	float drive;

} static s_filter;


static void FM_Moog_Ladder_Set_Cutoff(float cutoff)
{
	VIZ_ASSERT(cutoff >= 0.f && cutoff <= kAudibleNyquist);

	const float periodCut = (kPI*cutoff)/kSampleRate;
	const float gain = 4.f*kPI*kVT*cutoff*(1.f-periodCut)/(1.f+periodCut);
	
	s_filter.cutoff = cutoff;
	s_filter.periodCut = periodCut;
	s_filter.cutGain = gain;
}

static void FM_Moog_Ladder_Set_Resonance(float resonance)
{
	VIZ_ASSERT(resonance >= 0.f && resonance <= 4.f);
	s_filter.resonance = resonance;
}

static void FM_Moog_Ladder_ResetParameters()
{
	FM_Moog_Ladder_Set_Cutoff(1000.f); // Normalized cutoff frequency
	FM_Moog_Ladder_Set_Resonance(0.1f);
	s_filter.drive = 1.f;
}

static void FM_Moog_Ladder_Reset()
{
	for (auto iTap = 0; iTap < kTaps; ++iTap)
	{
		s_filter.V[iTap]  = 0.f;
		s_filter.dV[iTap] = 0.f;
		s_filter.tV[iTap] = 0.f;
	}
	
	FM_Moog_Ladder_ResetParameters();
}

#include "synth-taylor-tanhf.h"

static void FM_Apply_Moog_Ladder(float *pDest, unsigned numSamples)
{
	float dV0, dV1, dV2, dV3;

	const float resonance = s_filter.resonance;
	const float cutGain = s_filter.cutGain;
	const float drive = s_filter.drive;
	
	float *V  = s_filter.V;
	float *dV = s_filter.dV;
	float *tV = s_filter.tV;

	for (unsigned iSample = 0; iSample < numSamples; ++iSample)
	{
		dV0 = -cutGain * (FM_taylor_tanhf((drive*pDest[iSample] + resonance*V[3])*kThermalMul) + tV[0]);
		V[0] += (dV0 + dV[0]) / (2.f * kSampleRate);
		dV[0] = dV0;
		tV[0] = FM_taylor_tanhf(V[0]*kThermalMul);
			
		dV1 = cutGain * (tV[0] - tV[1]);
		V[1] += (dV1 + dV[1]) / (2.f * kSampleRate);
		dV[1] = dV1;
		tV[1] = FM_taylor_tanhf(V[1]*kThermalMul);
			
		dV2 = cutGain * (tV[1] - tV[2]);
		V[2] += (dV2 + dV[2]) / (2.f * kSampleRate);
		dV[2] = dV2;
		tV[2] = FM_taylor_tanhf(V[2]*kThermalMul);
			
		dV3 = cutGain * (tV[2] - tV[3]);
		V[3] += (dV3 + dV[3]) / (2.f * kSampleRate);
		dV[3] = dV3;
		tV[3] = FM_taylor_tanhf(V[3]*kThermalMul);
			
		pDest[iSample] = V[3];
	}
}
