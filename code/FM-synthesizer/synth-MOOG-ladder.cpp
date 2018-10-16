
/*
	Syntherklaas FM -- 4-pole MOOG ladder filter.

	Credits:
		- https://github.com/ddiakopoulos/MoogLadders/tree/master/src
		- Copyright 2012 Stefano D'Angelo <zanga.mail@gmail.com>
		- Corresponding paper by S. D'Angelo & V. Välimäki (2013)

	Filter does not require oversampling.
	Filter has been sligtly modified.

	FIXME: 
		- More than enough to optimize here if need be (SIMD, maybe), but let us wait for a target platform.
		- Consider using more precision or at least look at precision issues.
*/

#include "synth-global.h"
// #include "synth-MOOG-ladder.h"

namespace SFM
{
	namespace MOOG
	{
		// Thermal voltage (26 milliwats at room temperature)
		const float kVT = 0.312f;

		static float s_cutGain;
		static float s_resonance;
		static float s_drive;

		// Feedback values & deltas.
		static float s_V[4];
		static float s_dV[4];
		static float s_tV[4];

		void SetCutoff(float cutoff)
		{
			// Accept within normalized range
			SFM_ASSERT(cutoff >= 0.f && cutoff <= 1000.f); 

			const float omega = (kPI*cutoff)/kSampleRate;
			const float gain = 4.f*kPI*kVT*cutoff*(1.f-omega)/(1.f+omega);
	
			s_cutGain = gain;
		}

		void SetResonance(float resonance)
		{
			SFM_ASSERT(resonance >= 0.f && resonance <= kPI);
//			SFM_ASSERT(resonance >= 0.f && resonance <= 4.f);
			s_resonance = resonance;
		}

		void SetDrive(float drive)
		{
			s_drive = drive;
		}

		void ResetParameters()
		{
			SetCutoff(1000.f);
			SetResonance(0.1f);
			SetDrive(1.f);
		}

		void ResetFeedback()
		{
			for (unsigned iPole = 0; iPole < 4; ++iPole)
				s_V[iPole] = s_dV[iPole] = s_tV[iPole] = 0.f;
		}

		float Filter(float sample)
		{
			float dV0, dV1, dV2, dV3;

			const float &resonance = s_resonance;
			const float &cutGain = s_cutGain;
			const float &drive = s_drive;
			
			float *V  = s_V;
			float *dV = s_dV;
			float *tV = s_tV;

			dV0 = -cutGain * (fast_tanhf((sample*drive + resonance*V[3]) / (2.f*kVT)) + tV[0]);
			V[0] += (dV0+dV[0]) / (2.f*kSampleRate);
			dV[0] = dV0;
			tV[0] = fast_tanhf(V[0]/(2.f*kVT));
			
			dV1 = cutGain * (tV[0]-tV[1]);
			V[1] += (dV1+dV[1]) / (2.f*kSampleRate);
			dV[1] = dV1;
			tV[1] = fast_tanhf(V[1]/(2.f*kVT));
			
			dV2 = cutGain * (tV[1] - tV[2]);
			V[2] += (dV2+dV[2]) / (2.f*kSampleRate);
			dV[2] = dV2;
			tV[2] = fast_tanhf(V[2]/(2.f*kVT));
			
			dV3 = cutGain * (tV[2]-tV[3]);
			V[3] += (dV3+dV[3]) / (2.f*kSampleRate);
			dV[3] = dV3;
			tV[3] = fast_tanhf(V[3]/(2.f*kVT));

			// If this happens we're fucked
			SFM_ASSERT(false == IsNAN(V[3]));

			return V[3];
		}
	}
}
