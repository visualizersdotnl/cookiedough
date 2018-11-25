

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
	- Butterworth filter from http://www.musicdsp.org (see header file for details)
	- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
	- ADSR implementation by Nigel Redmon of earlevel.com (fixed and adjusted)

	Ideas & such:
		- Let's call the VST version the GENERALISSIMO!

	Right now:
		- Finish up delay, it's too predictable (including FIXMEs)
		- Bigger C:M table
		- Research gain tables because of volume issues

	Current goals (3-4 max. please):
		- Review fine & detune
		- Hard sync.
		- Filter envelope (or just version of voice envelope!)
		- Smarter way of building & supplying algorithms (i.e. a modulation matrix)
		  + Look at Kilian's work!

	Sound idea(s):
		- If a modulator is noisy, it sounds cool if the envelope is almost closed!
		- Consider operator targets instead of the current routing system

	Things that are missing or broken:
		- Cherry pick from the first iteration's lists
		- Potmeters crackle; I see no point in fixing this before I go for VST

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
