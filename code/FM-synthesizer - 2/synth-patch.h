
/*
	Syntherklaas FM -- "Dry" instrument patch.

	Dry means this does not contain a few global settings on the synthesizer such
	as LFO type, pitch envelope et cetera. These could quite easily be a part of the
	patch but I for now decide against it; a patch ideally sounds decent under a variety
	of global parameters. But, FIXME, this theory is yet to be tested.

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
			// Frequency setting; related to LUT (synth-LUT.cpp)
			unsigned coarse; 
			float fine;
			float detune;

			// Fixed frequency (alters interpretation of coarse and fine)
			// "If Osc Mode is set to FIXED FREQ (HZ), COARSE adjustment is possible in four steps--1, 10, 100 and
			// 1000. FINE adjustment is possible from 1 to 9.772 times."
			bool fixed;
			
			// Linear
			float amplitude;

			// Tremolo & vibrato
			float tremolo;
			float vibrato;

			// Amp. envelope
			float opEnvA;
			float opEnvD;

			// Velocity sensitivity (amplitude & envelope)
			float velSens;

			// Pitch envelope influence
			float pitchEnvAmt;

			// Level scaling settings
			// FIXME: optimize!
			unsigned levelScaleBP; // [0..127] MIDI
			float levelScaleLeft, levelScaleRight; // [-1..1]
 
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

				// Note tone
				OP.coarse = 0;
				OP.fine = 0.f;
				OP.detune = 0.f;

				// Not fixed ratio
				OP.fixed = false;

				// Amplitude/depth
				OP.amplitude = 0.f;

				// No tremolo/vibrato
				OP.tremolo = 0.f;
				OP.vibrato = 0.f;

				// Flat amplitude env.
				OP.opEnvA = 0.f;
				OP.opEnvD = 0.f;

				// Insensitive (amplitude & operator env.)
				OP.velSens = 0.f;

				// No pitch env. response
				OP.pitchEnvAmt = 0.f;

				// No level scaling
				OP.levelScaleBP = 69; // A4
				OP.levelScaleLeft = 0.f;
				OP.levelScaleRight = 0.f;

				// Zero feedback 
				OP.feedbackAmt = 0.f;
			}
		}
	};
}
