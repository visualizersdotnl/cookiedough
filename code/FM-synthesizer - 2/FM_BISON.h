

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

	To do:
		- Volume issues: introduce something called 'constant gain mixing'

	Ideas:
		- Allow carrier oscillator type (e.g. square, saw)
		- ADSR parameters: quick from 0.0 to 0.5, then a wider range follows after that (the Neutron has it)
		- Give filter it's own AD envelope
		- Consider operator targets instead of the current routing system
		- Hard sync. option
		- Smarter way of building & supplying algorithms (i.e. a modulation matrix)
		  + Not really all that necessary

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
