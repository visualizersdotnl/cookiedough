

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
		- Use ring buffer to see if tha takes care of the lag
		- Read more about proper cross fading
		- Try basic filter yourself before reintroducing the MOOG ladder

	Sound related: 
		- Implement own basic filter before using someone else's to get a better understanding
		- Review ADSR curves
		- Implement pink noise
		- Try cosine tilt envelope for shaping of modulator
		- Proper voice stealing

	Plumbing:
		- Use multiply-add in lerpf()
		- Debug log with formatting
		- Keep tracking NaN bugs

	Of later concern:
		- Optimization (LUTs, find hotspots using profiler)
		- Try Microtracker filter
		- Consider interpolated LUT sampler
		- Implement pitch bend
		- Take another gander at oscillators (clean ones), apply BLEP?
		- On that note (!), keep in mind that inlining isn't always as implicit as it should be
		- Double precision?
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
	unsigned TriggerNote(float frequency);

	// Release a note using it's voice index
	void ReleaseVoice(unsigned index);
}

#endif // _FM_BISON_H_
