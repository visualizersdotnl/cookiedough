
/*
	Syntherklaas FM -- Parameter state.
*/

#pragma once

#include "synth-global.h"
#include "synth-patch.h"
// #include "synth-ADSR.h"
#include "synth-filter.h"

namespace SFM
{
	// Most global parameters; some are still "pulled" in place (FIXME, see FM-BISON.cpp)
	struct Parameters
	{
		// Master drive [0..N]
		float drive;

		// Vibrato & tremolo [0..1]
		float vibrato;
		float tremolo;

		// Master ADSR
		ADSR::Parameters m_envParams;

		// Note jitter
		float m_noteJitter;

		// Global mod. index
		float modDepth;

		// Instr. patch
		FM_Patch patch;

		// Filter parameters
		int filterType; // 0 = Harsh, 1 = Soft
		float filterWet;
		FilterParameters filterParams;

		// Delay parameters
		float delayWet;
		float delayRate;
		float delayWidth;
		float delayFeedback;

		void SetDefaults()
		{
			// Neutral drive
			drive = 1.f;

			// No tremolo/vibrato
			tremolo = 0.f;
			vibrato = 0.f;

			// Std. ADSR
			m_envParams.attack  = 0.f;
			m_envParams.decay   = 0.25f;
			m_envParams.release = 0.25f;
			m_envParams.sustain = 1.f;

			// 50% jitter
			m_noteJitter = 0.5f;

			// Neutral modulation
			modDepth = 1.f;

			// Reset patches
			patch.Reset();

			// Filter off
			filterType = 0;
			filterWet = 0.f;
			filterParams.cutoff = 1.f;
			filterParams.resonance = 0.f;
			filterParams.drive = 1.f;

			// No delay
			delayWet = 0.f;
			delayRate = 0.f;
			delayWidth = 0.f;
			delayFeedback = 0.f;
		}
	};
}
