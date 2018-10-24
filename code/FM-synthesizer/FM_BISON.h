

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Credits due:
		- Magnus Jonsson for his Microtracker MOOG filter
		- D'Angelo & Valimaki for their MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
		- A nice ADSR implementation by Nigel Redmon of earlevel.com

	Notes:
	
	It's intended to be portable to embedded platforms in plain C if required (which it isn't now but close enough for an easy port), 
	and in part supplemented by hardware components if that so happens to be a good idea.

	I started programming in a very C-style but later realized that wasn't really necessary, but for now I stick to the chosen style
	as much as possible to keep it predictable to the reader; even if that means it's not very pretty in places.

	Things to do whilst not motivated (read: not hypomanic or medicated):
		- Not much, play a little, don't push yourself

	** CRACKLE BUG **

	Het heeft in elk geval iets te maken met het triggeren van voices, want als ik ze nooit terug geef heb ik hier geen last van.
	Waar je aan zou kunnen denken is dat er een threading probleem is; of wellicht zijn die gedeelde resources die globaal zijn
	niet helemaal jofel? Of het is iets doodsimpels, zul je zien.

	******************

	Working on:
		- Finetune ADSR(s) (use Nigel's as a reference)
		- VST preparation

	MIDI:
		- "Linear Smoothed Value" (see https://github.com/WeAreROLI/JUCE/tree/master/modules/juce_audio_basics/effects) for MIDI_Smoothed

	Sound related: 
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- Relate GetCarrierHarmonics() to input frequency, now it's just a number that "works"
		- Implement pitch bend
		- Improve wavetable oscillators
		- See about patches & more algorithms (careful)

	Plumbing:
		- Move MIDI calls out of FM. BISON, expose parameters through an object (preparation for VST)
		- Stash all oscillators in LUTs, makes it easier to switch or even blend between them and employ the same sampler quality
		- Move all math needed from Std3DMath to synth-math.h

	Of later concern:
		- Find hotspots (plenty!) and optimize
		- Eliminate floating point values where they do not belong and kill NAN-style bugs (major nuisance, causes crackle!)
		- Reinstate OSX port for on-the-go programming

	Bugs:
		- Master gain/drive isn't loud enough
		- Sometimes the digital saw oscillator cocks up (NAN)
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
