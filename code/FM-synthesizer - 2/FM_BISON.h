

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

	Until this feature set works and is reasonably efficient, no other features are to be added.

	Third party credits:
	- Transistor ladder filter impl. by Teemu Voipio (KVR forum)
	- Butterworth filter from http://www.musicdsp.org (see header file for details)
	- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
	- ADSR implementation by Nigel Redmon of earlevel.com (fixed and adjusted)

	Ideas:
		- Let's call the VST version the GENERALISSIMO!
		- For all notes on how to do and not to do things: refer to abandonded version

	Current goals (3-4 max.):
		- Review fine & detune
		- Bigger C:M table
		- Hard sync.?
		- Smarter way of building & supplying algorithms (i.e. a modulation matrix)

	Sound idea(s):
		- If a modulator is noisy, it sounds cool if the envelope is almost closed!

	Things that are missing:
		- Delay
		- Cherry pick from the first iteration's lists
	
	In short: get FM right, then reintroduce the goodie bag features

	Lessons already learned:
		- The DX7 is far more complex, but isn't it too complex?
		  If you want to implement that there's no way around LUTs and quasi-spaghetti, 
		  which is not the goal of this exercise
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
