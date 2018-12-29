
/*
	Syntherklaas FM -- Parameter state.
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

		void SetDefaults()
		{
			// Reset patch
			patch.Reset();

			// No bend
			pitchBend = 0.f;
		}
	};
}
