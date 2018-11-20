

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Third party credits: see previous iteration

	Ideas:
		- Let's call the VST version the GENERALISSIMO!
		- For all notes on how to do and not to do things: refer to abandonded version

	Current goals (3-4 max.):
		- Bigger C:M table
		- Hard sync.?
		- Smarter way of building & supplying algorithms

	Things that are missing:
		- Proper LFO
		- Chorus
	
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
