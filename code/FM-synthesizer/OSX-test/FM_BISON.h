
/*
	Syntherklaas FM presents 'FM. BISON'
	(C) syntherklaas.org, a subsidiary of visualizers.nl

	This is intended to be a powerful yet relatively simple FM synthesizer core.

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

	Priority / Bugs:
		- Work out Vorticity
		- Check up on BLIT harmonics amount
		- Finish up ADSR (see impl.)
		- Implement a master rotary or fader to scale voice amplitude ("main")	
		- Get a clearer picture of "generic" LFOs and implement a few
		- Smooth out MIDI controls using Maarten van Strien's trick (interpolate 64 samples until next value)
		- Use ring buffer to feed
		- Consider interpolated LUT sampler

	To do:
		- Take another gander at oscillators (clean ones), add a few noise types
		- Implement LFOs: form, period, frequency, aplitude, and apply it to modulation index first
		- Implement pitch bend
		- For now it is convenient to keep modulators and carriers apart but they might start sharing too much logic
		  to keep it this way
		- Optimization (LUTs, branching et cetera)
		- On that note (!), keep in mind that inlining isn't always as implicit as it should be
		- Keep tracking NAN bugs.
		- See notebook
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

	// Trigger a note (if possible) and return it's voice index
	unsigned TriggerNote(unsigned midiIndex);

	// Release a note using it's voice index
	void ReleaseNote(unsigned iVoice);
}

/*

// To feed BASS (see Bevacqua's audio.h):
#include "../audio.h"
DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser);

*/

#endif // _FM_BISON_H_
