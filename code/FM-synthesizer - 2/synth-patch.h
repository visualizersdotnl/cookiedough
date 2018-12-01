
/*
	Syntherklaas FM -- "Dry" instrument patch.

	Volca (unofficial) manual: http://afrittemple.com/volca/volca_programming.pdf

	I cite:

	"If Osc Mode is set to FIXED FREQ (HZ), COARSE adjustment is possible in four steps--1, 10, 100 and
	1000. FINE adjustment is possible from 1 to 9.772 times."
*/

#pragma once

namespace SFM
{
	const float kFixedFineScale = 9.772f;

	struct FM_Patch
	{
		// [0..1] unless stated otherwise
		struct Operator
		{
			// Related to LUT (synth-LUT.cpp)
			unsigned coarse; 
			float fine;
			float detune;

			// Fixed frequency (alters interpretation of coarse and fine)
			bool fixed;
			
			// Linear
			float amplitude;

			// Tremolo & vibrato
			float tremolo;
			float vibrato;

			// Amp. envelope
			float opEnvA;
			float opEnvD;

			// Feedback amount
			// This only has effect when operator is used by itself or another in a feedback loop
			float feedbackAmt;
		};
	
		Operator operators[kNumOperators];

		void Reset()
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			{	
				Operator &OP = operators[iOp];
				OP.coarse = 1;
				OP.fine = 0.f;
				OP.detune = 0.f;
				OP.fixed = false;
				OP.amplitude = 1.f;
				OP.tremolo = 0.f;
				OP.vibrato = 0.f;
				OP.opEnvA = 0.f;
				OP.opEnvD = 0.f;
				OP.feedbackAmt = 0.f;
			}
		}
	};
}
