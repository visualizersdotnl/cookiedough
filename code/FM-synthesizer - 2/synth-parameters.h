
/*
	Syntherklaas FM -- Parameter state.
*/

#pragma once

#include "synth-global.h"
#include "synth-patch.h"
#include "synth-ADSR.h"

namespace SFM
{
	// Most global parameters; some are still "pulled" in place (FIXME)
	struct Parameters
	{
		// Master drive [0..N]
		float drive;

		// Master vibrato [0..1]
		float vibrato;

		// Master ADSR
		ADSR::Parameters m_envParams;

		// Global mod. index
		float modDepth;

		// Instr. patch
		Patch patch;

		void SetDefaults()
		{
			// Neutral drive
			drive = 1.f;

			// No vibrato
			vibrato = 0.f;

			// Std. ADSR
			m_envParams.attack  = 0.f;
			m_envParams.decay   = 0.25f;
			m_envParams.release = 0.25f;
			m_envParams.sustain = 1.f;

			// Neutral modulation
			modDepth = 1.f;

			// Reset patches
			patch.Reset();
		}
	};
}
