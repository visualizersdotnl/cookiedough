
/*
	Syntherklaas FM -- Parameter state.
*/

#pragma once

#include "synth-global.h"
#include "synth-patch.h"

namespace SFM
{
	// Most global parameters; some are still "pulled" in place (FIXME)
	struct Parameters
	{
		// Master drive [0..N]
		float drive;

		// Global mod. index
		float modDepth;

		// Instr. patch
		Patch patch;

		void SetDefaults()
		{
			// Neutral drive
			drive = 1.f;

			// Neutral modulation
			modDepth = 1.f;

			// Reset patches
			patch.Reset();
		}
	};
}
