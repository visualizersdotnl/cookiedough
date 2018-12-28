
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

		void SetDefaults()
		{
			// Reset patch
			patch.Reset();
		}
	};
}
