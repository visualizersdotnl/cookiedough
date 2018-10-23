

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	I have to thank the following people for helping me with their knowledge, ideas and experience:
		- Pieter v/d Meer
		- Maarten van Strien (Crystal Score/TBL)
		- Ronny Pries (Ronny Pries/Farbrausch)
		- Tammo Hinrichs (KB/Farbarausch)
		- Alex Bartholomeus (Deadline/Superstition)
		- Thorsten �rts (Thorsten/Purple)
		- Stijn Haring-Kuipers
		- Zden Hlinka (Zden/Satori)

	Thanks to:
		- Magnus Jonsson for his Microtracker MOOG filter impl. on which on of ours is based
		- The KVR forum

	Notes:
	
	It's intended to be portable to embedded platforms in plain C if required (which it isn't now but close enough for an easy port), 
	and in part supplemented by hardware components if that so happens to be a good idea.

	I started programming in a very C-style but later realized that wasn't really necessary, but for now I stick to the chosen style
	as much as possible to keep it predictable to the reader; even if that means it's not very pretty in places.

	Things to do whilst not motivated (read: not hypomanic or medicated):
		- Not much, play a little, don't push yourself

	Working on:

	To do:
		- Try to get current state of MIDI controls on startup in driver
		- "Linear Smoothed Value" (see https://github.com/WeAreROLI/JUCE/tree/master/modules/juce_audio_basics/effects) for MIDI
		+ Both low-priority

	Sound related: 
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- Relate GetCarrierHarmonics() to input frequency, now it's just a number that "works"
		- Implement pitch bend?

	Proper sound related task:
		- Create algorithms & patches; just start by emulating something well known

	Plumbing:
		- Move MIDI calls out of FM. BISON, expose parameters through an object (preparation for VST)
		- Stash all oscillators in LUTs, makes it easier to switch or even blend between them and employ the same sampler quality
		- Move all math needed from Std3DMath to synth-math.h

	Of later concern:
		- Clean up Oxygen 49 driver so it's easier to port to another keyboard (or use an existing library)
		- Eliminate floating point values where they do not belong and kill NAN-style bugs (major nuisance, causes crackle!)
		- Reinstate OSX port for on-the-go programming

	The Idea Bin:
		- Operator switches for envelopes

	Bugs:
		- Sometimes the digital saw oscillator cocks up

	Simon Cann's useful videos:
		- https://www.youtube.com/watch?v=95Euc8SdTyc
*/

#ifndef _FM_BISON_H_
#define _FM_BISON_H_

#include "synth-global.h"
#include "synth-oscillators.h"

bool Syntherklaas_Create();
void Syntherklaas_Destroy();

// Returns loudest voice (linear amplitude)
float Syntherklaas_Render(uint32_t *pDest, float time, float delta);

namespace SFM
{
	/*
		API exposed to (MIDI) input.
	*/

	// Trigger a note (if possible) and return it's voice index: at this point it's a voice
	unsigned TriggerNote(Waveform form, float frequency, float velocity);

	// Release a note using it's voice index
	void ReleaseVoice(unsigned index);
}

#endif // _FM_BISON_H_
