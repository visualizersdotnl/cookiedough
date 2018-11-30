

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	This is my second attempt at an FM synthesizer:
	- 6 programmable operators with modulation envelope and shared tremolo & vibrato LFOs
	- Master drive & ADSR
	- Master clean 24dB & MOOG 24dB ladder filters
	- Global modulation control
	- Pitch bend
	- Multiple tried & tested FM algorithms
	- Paraphonic (24 voices)
	- Adjustable tone jitter (for analogue VCO warmth)
	- Tuneable global delay effect

	In the works:
	- LFO key sync. switch

	Until this feature set works and is reasonably efficient, no other features are to be added.
	At this point this code is not optimized for speed but there's tons of low hanging fruit.

	Third party credits:
	- Transistor ladder filter impl. by Teemu Voipio (KVR forum)
	- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
	- ADSR implementation by Nigel Redmon of earlevel.com (fixed and adjusted)

	Missing for near-DX7/Volca compatibility:
		- LFO key sync (working on it)
		- Tweakable sensitivity to velocity (per operator?)
		- What's this fixed frequency deal good for?
		- More algorithms!

	Maybes for the above:
		- More elaborate operator envelope
		- Global operator envelope *also* applied to pitch LFO

	Tasks: 
		- Look at C:M issue a little closer
		- Build decent algorithm hardcoding system
		- Check parameter ranges
		- Review operator loop

	Ideas:
		- Operator hard. sync option?

	Things that are missing or broken:
		- DX detune is now 0 to 14 semitones instead of -7 to +7 due to stupid MIDI potmeter
		- "Clean" filter plops when cutoff is pulled shut
		- Mod. wheel should respond during note playback
		- Potmeters crackle; I see no point in fixing this before I go for VST
		- Cherry pick from the first iteration's lists

	Lessons learned:
		- Complexity has been done, it's not the answer
		- Synth. coding is hard for a beginner, give yourself time
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
