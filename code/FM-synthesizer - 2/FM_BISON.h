

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	This is my second attempt at an FM synthesizer:
	- 6 programmable operators with envelope and shared tremolo & vibrato LFOs
	- Master drive & ADSR
	- Master clean 24dB & MOOG 24dB ladder filters
	- Global modulation control
	- Pitch bend
	- Multiple tried & tested FM algorithms
	- Paraphonic (24 voices)
	- Adjustable tone jitter (for analogue warmth)
	- Tuneable global delay effect

	Until this feature set works and is reasonably efficient, no other features are to be added.
	At this point this code is not optimized for speed but there's tons of low hanging fruit.

	Third party credits:
	- Transistor ladder filter impl. by Teemu Voipio (KVR forum)
	- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
	- ADSR implementation by Nigel Redmon of earlevel.com (fixed and adjusted)

	Missing for near-DX7/Volca compatibility:

	* I am losing terrain by not having some sharpness in the first phase.
	* My operator envelope modulates both tremolo, pitch and amplitude; this might not be the way to go.
	  The Volca FM seems to only modulate pitch with an outside envelope.
	* My operators do not support per operator velocity sensitivity.
	* I have no fixed frequency operators.
	* I lack algorithms.
	* This 'level scale' deal is a way of making envelopes non-linear; I should understand it a little better.

	Tasks for today:
		- Build decent algorithm hardcoding system

	For later:
		- Check parameter ranges
		- Review operator loop
		- All FIXMEs

	Things that are missing or broken:
		- "Clean" filter plops when cutoff is pulled shut: why?
		- Mod. wheel should respond during note playback
		- Potmeters crackle; I see no point in fixing this before I go for VST
		- Cherry pick from the first iteration's lists
*/

#pragma once

#include "synth-global.h"
#include "synth-stateless-oscillators.h"

bool Syntherklaas_Create();
void Syntherklaas_Destroy();

// Returns loudest voice (linear amplitude)
float Syntherklaas_Render(uint32_t *pDest, float time, float delta);

namespace SFM
{
	/*
		API exposed to (MIDI) input.
		I'm assuming all TriggerVoice() and ReleaseVoice() calls will be made from the same thread, or at least not concurrently.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, float frequency, float velocity);
	void ReleaseVoice(unsigned index, float velocity /* Aftertouch */);
}
