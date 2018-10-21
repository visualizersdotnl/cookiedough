

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

	Things to do whilst not motivated (read: hypomanic or medicated):
		- Not much, play a little, don't push yourself

	To do:
		- See Moleskine (will overlap with items below)
		- Try to get current state of MIDI controls on startup in driver
		- Try and filter MIDI inputs
		- If sustain is zero, skip that stage of the ADSR?
		- Try basic filter yourself (start with 1-pole LP) before reintroducing the MOOG ladder

	Sound related: 
		- More elaborate voice stealing (base on velocity, says Pieter)
		- Influence ADSR by velocity
		- Read more about proper cross fading
		- Review ADSR (by someone else than me)
		- Implement pink noise
		- Try cosine tilt envelope for shaping of modulator
		- Proper voice stealing
		- Relate GetCarrierHarmonics() to input frequency, now it's just a number that "works"

	Plumbing:
		- Debug log with formatting
		- Move all math needed from Std3DMath to synth-math.h
		- Stash all oscillators in LUTs, makes it easier to switch or even blend between them and employ the same sampler quality
		- Clean up Oxygen 49 driver so it's easier to port to another keyboard

	Of later concern:
		- Implement pitch bend
		- Optimization (LUTs, find hotspots using profiler); probably eliminate floating point values where they don't belong
		- Clipping and master drive (or gain if you will) has been resolved enough, *for now*, but ask around
		- Keep tracking NaN bugs
		- Double precision?
		- Reinstate OSX port for on-the-go programming
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
