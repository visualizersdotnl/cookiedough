

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	This is my second attempt at an FM synthesizer:
	- 6 programmable operators with modulation envelope and shared tremolo & vibrato LFOs
	- Master drive & ADSR
	- Master Butterworth (sharp & clear) & MOOG (soft) ladder filters
	- Global modulation control
	- Pitch bend
	- Multiple tried & tested FM algorithms
	- Polyphonic (24 voices)
	- Adjustable tone jitter (for analogue VCO warmth)
	- Tuneable global delay effect

	Until this feature set works and is reasonably efficient, no other features are to be added.
	At this point this code is not optimized for speed but there's tons of low hanging fruit.

	Third party credits:
	- Transistor ladder filter impl. by Teemu Voipio (KVR forum)
	- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
	- ADSR implementation by Nigel Redmon of earlevel.com (fixed and adjusted)

	Ideas & such:
		- Let's call the VST version the GENERALISSIMO!

	Priority bug(s): 
		- My attack-decay envelope makes NO sense at all, I am always attacking to 1 and decaying in the same direction,
		  whereas I feel if decay is zero I should stay at 1, and the longer it takes the more I should go towards a zero sustain
		- Is my feedback right like this or should I give it some overdrive and a filter?
		- Review the voice loop in any case: is your order of operations right?

	To do:
		- Volume issues: introduce something called 'constant gain mixing'

	Ideas:
		- Allow carrier oscillator type (e.g. square, saw)
		- Consider operator targets instead of the current routing system (which is a tad overcomplicated)
		- Hard sync. option
		- Smarter way of building & supplying algorithms (i.e. a modulation matrix)
		  + Not really all that necessary

	Important for VST:
		- DX detune is now 0 to 14 semitones instead of -7 to +7 due to stupid MIDI potmeter

	Things that are missing or broken:
		- Mod. wheel should respond during note playback?
		- Potmeters crackle; I see no point in fixing this before I go for VST
		- Cherry pick from the first iteration's lists

	Lessons learned:
		- Complexity has been done, it's not the answer
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
