

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	I have to thank the following people for helping me with their knowledge, ideas and experience:
		- Pieter v/d Meer
		- Ronny Pries (Ronny Pries/Farbrausch)
		- Tammo Hinrichs (KB/Farbarausch)
		- Alex Bartholomeus (Deadline/Superstition)
		- Thorsten Ørts (Thorsten/Purple)
		- Stijn Haring-Kuipers
		- Zden Hlinka (Zden/Satori)

	Notes:
	
	It's intended to be portable to embedded platforms in plain C if required (which it isn't now but close enough for an easy port), 
	and in part supplemented by hardware components if that so happens to be a good idea.
	
	So the style will look a bit dated here with a few modern bits there, but nothing major.

	Things to do whilst not motivated (read: manic or medicated):
		- Fix that darned voice mixing once and for all
		- Try basic filter yourself (start with 1-pole LP) before reintroducing the MOOG ladder
		- Try and filter MIDI inputs
		- More elaborate voice stealing (base on velocity, says Pieter)
		- Influence ADSR by velocity
		- Create a simple disk writer that gathers samples when a button is pushed (MIDI) and dumps it on release (!)
		- Read more about proper cross fading

	Sound related: 
		- Review & optimize ADSR
		- Implement pink noise
		- Try cosine tilt envelope for shaping of modulator
		- Proper voice stealing

	Plumbing:
		- Stash all oscillators in LUTs, makes it easier to switch or even blend between them and employ the same sampler quality
		- Debug log with formatting
		- Move all math needed from Std3DMath to synth-math.h

	Of later concern:
		- Optimization (LUTs, find hotspots using profiler)
		- Implement pitch bend
		- Double precision
		- Keep tracking NaN bugs
*/

#ifndef _FM_BISON_H_
#define _FM_BISON_H_

#include "synth-global.h"

bool Syntherklaas_Create();
void Syntherklaas_Destroy();
void Syntherklaas_Render(uint32_t *pDest, float time, float delta);

namespace SFM
{
	/*
		API exposed to (MIDI) input.
	*/

	// Trigger a note (if possible) and return it's voice index: at this point it's a voice
	unsigned TriggerNote(float frequency, float velocity);

	// Release a note using it's voice index
	void ReleaseVoice(unsigned index);
}

#endif // _FM_BISON_H_
