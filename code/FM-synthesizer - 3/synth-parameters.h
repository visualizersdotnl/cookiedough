
/*
	Syntherklaas FM -- Parameter state.

	I intend to keep all state [0..1] floating point for easy VST integration.
*/

#pragma once

#include "synth-global.h"
#include "synth-patch.h"

namespace SFM
{
	struct Parameters
	{
		// Dry instr. patch
		FM_Patch patch;

		// Pitch bend
		float pitchBend;

		// Modulation
		float modulation;

		// LFO speed
		float LFOSpeed;

		void SetDefaults()
		{
			// Reset patch
			patch.Reset();

			// No bend
			pitchBend = 0.f;

			// No modulation
			modulation = 0.f;
			LFOSpeed = 0.f;
		}
	};
}
