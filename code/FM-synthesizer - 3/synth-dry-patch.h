
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

			// Output level
			float output;

			// ADSR
			float attack;
			float decay;
			float sustain;
			float release;
			float attackLevel;

			// Velocity sensitivity
			float velSens;

			// Feedback amount
			float feedback;

			// LFO influence (on the DX7 this is global)
			float ampMod;
			float pitchMod;

			// Distortion
			float distortion;

			// Level scaling (DX7-style, simplified)
			unsigned levelScaleBP;
			unsigned levelScaleRange; // [0..127]
			float levelScaleL, levelScaleR;

			// Envelope rate multiplier
			// Range taken from Arturia DX7-V (http://downloads.arturia.com/products/dx7-v/manual/dx7-v_Manual_1_0_EN.pdf)
			float envRateMul; // [0.100..10.0]

			// Envelope keyboard rate scaling
			float envRateScale;
		};
	
		Operator operators[kNumOperators];

		// Shared pitch envelope
		bool pitchEnvInvert;   // If true invert samples (1-value).
		float pitchEnvBias;    // 0.0 = completely positive ([0..1.0]), 1.0 = completely negative ([0..-1.0]).
		float pitchEnvAttack;
		float pitchEnvDecay;
		float pitchEnvSustain;
		float pitchEnvRelease;
		float pitchEnvLevel;

		void Reset()
		{
			// No pitch envelope
			pitchEnvInvert = false;
			pitchEnvBias = 0.f;
			pitchEnvAttack = 0.f;
			pitchEnvDecay = 0.f;
			pitchEnvRelease = 0.f;
			pitchEnvLevel = 1.f;

			for (auto &patchOp : operators)
			{	
				// Note tone (0 == frequency/2)
				patchOp.coarse = 1;
				patchOp.fine = 0.f;
				patchOp.detune = 0.f;

				// Ratio
				patchOp.fixed = false;

				// Output level
				patchOp.output = 1.f;

				// No envelope
				patchOp.attack = 0.f;
				patchOp.decay = 0.f;
				patchOp.sustain = 1.f;
				patchOp.release = 0.f;
				patchOp.attackLevel = 1.f;

				// Not velocity sensitive
				patchOp.velSens = 0.f;

				// No feedback
				patchOp.feedback = 0.f;

				// No LFO influence
				patchOp.ampMod = 0.f;
				patchOp.pitchMod = 0.f;

				// No distortion
				patchOp.distortion = 0.f;

				// No level scaling
				patchOp.levelScaleBP = 69; // A4
				patchOp.levelScaleRange = 24; // 2 octaves
				patchOp.levelScaleL = 0.f;
				patchOp.levelScaleR = 0.f;

				// Env. rate multiplier
				patchOp.envRateMul = 1.f;

				// No envelope keyboard scaling
				patchOp.envRateScale = 0.f;
			}
		}
	};
}
