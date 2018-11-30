
/*
	Syntherklaas FM -- "Dry" instrument patch.
*/

#pragma once

namespace SFM
{
	struct FM_Patch
	{
		// [0..1] unless stated otherwise
		struct Operator
		{
			// Freq. modifiers (http://afrittemple.com/volca/volca_programming.pdf)
			unsigned coarse; // Related to LUT (synth-LUT.cpp)
			float fine;
			float detune;
			
			// Linear
			float amplitude;

			// Tremolo & vibrato
			float tremolo;
			float vibrato;

			// Simple envelope
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
				OP.coarse = 0;
				OP.fine = 0.f;
				OP.detune = 0.f;
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
