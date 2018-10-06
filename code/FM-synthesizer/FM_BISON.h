
/*
	Syntherklaas FM presents 'FM. BISON'
	(C) syntherklaas.org, a subsidiary of visualizers.nl

	This is intended to be a powerful yet relatively simple FM synthesizer core.

	I have to thank the following people for helping me with their knowledge, ideas and experience:
		- Ronny Pries
		- Tammo Hinrichs
		- Alex Bartholomeus
		- Pieter v/d Meer
		- Thorsten Ørts
		- Stijn Haring-Kuipers
		- Dennis de Bruijn
		- Zden Hlinka

	It's intended to be portable to embedded platforms in plain C (which it isn't now but close enough), 
	and in part supplemented by hardware components if that so happens to be a good idea.
	So the style will look a bit dated here and there.

	Priority / /Bugs:
		- Use ring buffer to feed
		- Use float exceptions to track NAN-bugs (happens when MOOG-filtering)
		- ADSR on voices (uses flexible ADSR based on velocity)
		- Smooth out MIDI controls using Maarten van Strien's trick (interpolate 64 samples until next value)

	To do:
		- Normalize volumes as we go?
		- For now it is convenient to keep modulators and carriers apart but they might start sharing too much logic
		  to keep it this way.
		- Optimization, FIXMEs, interpolation.
		- See notebook.
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
		API exposed to MIDI input.
	*/

	// Trigger a note (if possible) and return it's internal index
	unsigned TriggerNote(unsigned midiIndex);

	// Release a note using it's internal index
	void ReleaseNote(unsigned index);
}

// To feed BASS (see audio.h):
#include "../audio.h"
DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser);

#endif // _FM_BISON_H_
