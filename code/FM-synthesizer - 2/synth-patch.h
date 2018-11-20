
/*
	Syntherklaas FM -- Instrument patch.
*/

#pragma once

namespace SFM
{
	struct Patch
	{
		struct Operator
		{
			// Freq. modifiers (http://afrittemple.com/volca/volca_programming.pdf)
			unsigned coarse; // Related to LUT (synth-LUT.cpp)
			float fine;
			float detune;
			
			// [0..1]
			float amplitude;
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
			}
		}
	};
}
