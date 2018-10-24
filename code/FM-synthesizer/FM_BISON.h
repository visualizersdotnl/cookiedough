

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Third-party code used:
		- Magnus Jonsson's Microtracker MOOG filter
		- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
		- ADSR implementation by Nigel Redmon of earlevel.com

	Notes:

		- The code style started out as semi-C, intending a hardware target, but though I try to stick to that particular style since
		  it's there I'm realizing I can just as well use more C++ since the target is never going to be very archaic. So if it comes
		  across as inconsistent here and there: that's because it is.

	Things to do whilst not motivated (read: not hypomanic or medicated):
		- Not much, play a little, don't push yourself

	Working on:
		- The worst crackle and pop is gone but I'm not convinced yet
		- Write wrapper class for wavetable oscillators
		- Feed envelope to filters, adjust the second one to use fast_tanhf()
		- Finetune ADSR(s)
		- VST preparation

	MIDI:
		- "Linear Smoothed Value" (see https://github.com/WeAreROLI/JUCE/tree/master/modules/juce_audio_basics/effects) for MIDI_Smoothed

	Sound related: 
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- Implement pitch bend
		- Improve oscillators (blend)
		- Relate GetCarrierHarmonics() to input frequency, now it's just a number that "works"
		- See about patches & more algorithms (careful)

	Plumbing:
		- Turn structures into real classes
		- Move MIDI calls out of FM. BISON, expose parameters through an object (part of preparation for VST)
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
