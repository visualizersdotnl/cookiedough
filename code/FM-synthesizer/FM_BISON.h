

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

	Actual credits:
		- Magnus Jonsson for his Microtracker MOOG filter impl. on which on of ours is based

	Notes:
	
	It's intended to be portable to embedded platforms in plain C if required (which it isn't now but close enough for an easy port), 
	and in part supplemented by hardware components if that so happens to be a good idea.
	
	So the style will look a bit dated here with a few modern bits there, but nothing major.

	Things to do whilst not motivated (read: hypomanic or medicated):
		- Not much, play a little, don't push yourself

	To do:
		- Try to get current state of MIDI controls on startup in driver
		- Try using kMaxVoicedB

	Sound related: 
		- More elaborate voice stealing (base on velocity, says Pieter)
		- Implement pink noise
		- Relate GetCarrierHarmonics() to input frequency, now it's just a number that "works"
		- Implement pitch bend?

	Proper sound related task:
		- Create algorithms & patches; just start by emulating the electric piano as we all know it

	Plumbing:
		- Stash all oscillators in LUTs, makes it easier to switch or even blend between them and employ the same sampler quality
		- Move all math needed from Std3DMath to synth-math.h
		- Optimized copy to ring buffer in Render()

	Of later concern:
		- Clipping and master drive (or gain if you will) has been resolved enough, *for now*, but ask around
		- If sustain is zero, skip that stage of the ADSR? Purely an optimization since you can do it just with the right settings.
		- Clean up Oxygen 49 driver so it's easier to port to another keyboard (or use an existing library)
		- Eliminate floating point values where they do not belong and kill NAN-style bugs
		- Reinstate OSX port for on-the-go programming

	The Idea Bin:
		- Cosine tilt envelope to modulate FM, with 2 controls and 1 or more operator switches
		- Periodical ADSR envelope to modulate cutoff, off and on switch to begin with, plus A and R faders
		- Modulation ratio fader (<3)
		- Influence ADSR by velocity (says Ronny)
		- Modulate cutoff

	Simon Cann's useful videos:
		- https://www.youtube.com/watch?v=95Euc8SdTyc
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
