
/*
	Syntherklaas FM -- Parameter state.

	I intend to keep all state [0..1] floating point for easy VST integration.
*/

#pragma once

#include "synth-global.h"
#include "synth-dry-patch.h"

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

		// Liveliness
		float liveliness;

		// LFO key sync.
		bool LFOSync;

		// Chorus 
		bool chorus;

		// Filter parameters
		float cutoff;
		float resonance;

		// Pickup parameters
		float pickupDist;
		float pickupAsym;

		void SetDefaults()
		{
			// Reset patch
			patch.Reset();

			// No bend
			pitchBend = 0.f;

			// No modulation
			modulation = 0.f;
			LFOSpeed = 0.f;

			// Sync.
			LFOSync = true;

			// 100% straight
			liveliness = 0.f;

			// No chorus
			chorus = false;

			// Little to no filtering
			cutoff = 1.f;
			resonance = 1.f;

			// Pickup parameters
			pickupDist = 0.5f;
			pickupAsym = 1.f;
		}
	};
}
