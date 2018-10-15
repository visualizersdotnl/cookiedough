
/*
	Syntherklaas FM presents 'FM. BISON'
	by syntherklaas.org, a subsidiary of visualizers.nl

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
		- Add drive control (and figure out acceptable range) to MOOG ladder
		- Try Microtracker filter)
		- Backport immediately!
		- Check ADSR curves
		- Use cosine tilt envelope for carrier freq. shaping
		- Use ring buffer
		- Debug log with formatting
		- Implement pink noise (and possibly, later, Thorsten's noise)
		- Steer vorticity by MIDI control (just 1 would be best, as not to overcomplicate things)
		- Implement global gain (or 'drive')
		- Keep tracking NAN bugs.

	To do:
		- Check MOOG filter cutoff (why does it detune at higher frequencies and cause self-oscillation?)
		- Optimization (LUTs, find hotspots using profiler)
		- Use multiply-add in lerpf()

	Consider:
		- Consider interpolated LUT sampler
		- Implement pitch bend
		- Take another gander at oscillators (clean ones), apply BLEP?
		- On that note (!), keep in mind that inlining isn't always as implicit as it should be
		- Double precision
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
