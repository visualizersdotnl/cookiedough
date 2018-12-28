
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

			// ADSR
			float attack;
			float decay;
			float sustain;
			float release;

			// Velocity sensitivity
			float velSens;
		};
	
		Operator operators[kNumOperators];

		void Reset()
		{
			for (auto &patchOp : operators)
			{	
				// Note tone (0 == frequency/2)
				patchOp.coarse = 1;
				patchOp.fine = 0.f;
				patchOp.detune = 0.f;

				// Ratio
				patchOp.fixed = false;

				// Depth/Amplitude
				patchOp.index = 1.f;

				// No envelope
				patchOp.attack = 0.f;
				patchOp.decay = 0.f;
				patchOp.sustain = 1.f;
				patchOp.release = 0.f;

				// Not velocity sensitive
				patchOp.velSens = 0.f;
			}
		}
	};
}
