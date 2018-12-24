
/*
	Syntherklaas FM -- Parameter state.
*/

#pragma once

#include "synth-global.h"
#include "synth-patch.h"
// #include "synth-ADSR.h"
#include "synth-filter.h"
#include "synth-stateless-oscillators.h"
#include "synth-vowel-filter.h"

namespace SFM
{
	// Most global parameters; some are still "pulled" in place (FIXME, see FM-BISON.cpp)
	struct Parameters
	{
		// Master drive [0..N]
		float drive;

		// Tremolo [0..1]
		float tremolo;

		// Master ADSR
		ADSR::Parameters envParams;

		// Note jitter
		float noteJitter;

		// LFO type
		Waveform LFOform;

		// Chorus type
		unsigned chorusType;

		// Instr. patch
		FM_Patch patch;

		// Filter parameters
		bool filterInv;
		ADSR::Parameters filterEnvParams;
		int filterType; // 0 = Harsh, 1 = Soft
		float filterWet;
		FilterParameters filterParams;

		// Delay parameters
		float delayWet;
		float delayRate;

		// Pitch env.
		float pitchA;
		float pitchD;
		float pitchL;

		// Vowel filter
		float vowelWet;
		float vowelBlend;
		VowelFilter::Vowel vowel;

		// Pitch bend
		float pitchBend;

		// Vibrato
		float vibrato;

		void SetDefaults()
		{
			// Neutral drive
			drive = 1.f;

			// No tremolo
			tremolo = 0.f;

			// Std. ADSR
			envParams.attack       = 0.f;
			envParams.attackLevel  = 1.f;
			envParams.decay        = 0.25f;
			envParams.release      = 0.25f;
			envParams.sustainLevel = 1.f;

			// No jitter
			noteJitter = 0.f;

			// Cosine LFOs
			LFOform = Waveform::kSine;

			// Chorus type 1
			chorusType = 0;

			// Reset patches
			patch.Reset();

			// Filter off
			filterInv = false;
			filterEnvParams = envParams;
			filterType = 0;
			filterWet = 0.f;
			filterParams.cutoff = 1.f;
			filterParams.resonance = 0.f;
			filterParams.drive = 1.f;

			// No delay
			delayWet = 0.f;
			delayRate = 0.f;

			// No pitch env.
			pitchA = 0.f;
			pitchD = 0.f;
			pitchL = 0.f;

			// No vowel filter
			vowelWet = 0.f;
			vowelBlend = 0.f;
			vowel = VowelFilter::kA;

			// Zero bend
			pitchBend = 1.f; // 1.f == powf(2.f, 0.f)

			// No vibrato
			vibrato = 0.f;
		}
	};
}
