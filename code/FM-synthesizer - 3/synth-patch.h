
/*
	Syntherklaas FM -- "Dry" instrument patch.

	Dry means this does not contain *global* settings (see synth-parameters.h).
	Ideally a patch sounds right on it own, under any combination of global settings.

	Volca (unofficial) manual: http://afrittemple.com/volca/volca_programming.pdf
*/

#pragma once

namespace SFM
{
	// Max. amount of fine tuning for fixed ratio operators (source: Volca FM)
	const float kFixedFineScale = 9.772f;

	struct FM_Patch
	{
		// [0..1] unless stated otherwise
		struct Operator
		{
			// Frequency settings
			unsigned coarse; 
			float fine;
			float detune;

			// Fixed frequency (alters interpretation of coarse and fine)
			// "If Osc Mode is set to FIXED FREQ (HZ), COARSE adjustment is possible in four steps--1, 10, 100 and
			//  1000. FINE adjustment is possible from 1 to 9.772 times."
			bool fixed;
			
			// Depth/Amplitude
			float index;

			// Velocity sensitivity
			float velSens;
		};
	
		Operator operators[kNumOperators];

		void Reset()
		{
			for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			{	
				Operator &OP = operators[iOp];

				// Note tone (0 == frequency/2)
				OP.coarse = 1;
				OP.fine = 0.f;
				OP.detune = 0.f;

				// Ratio
				OP.fixed = false;

				// Depth/Amplitude
				OP.index = 1.f;

				// Not velocity sensitive
				OP.velSens = 0.f;
			}
		}
	};
}
