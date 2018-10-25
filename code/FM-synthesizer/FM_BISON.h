

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Third-party code used:
		- Magnus Jonsson's Microtracker MOOG filter
		- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
		- ADSR implementation by Nigel Redmon of earlevel.com

	Notes:
		- The code style started out as semi-C, intending a hardware target, currently in the process of cleaning it up a bit
		  and upgrading to somewhat proper C++.
		- You'll run into various numbers and seemingly weird calculations: it's called "bro science".
		- On FM ratios: http://noyzelab.blogspot.com/2016/04/farey-sequence-tables-for-fm-synthesis.html


	Things to do whilst not motivated (read: not hypomanic or medicated):
		- Not much, play a little, don't push yourself

	Priority:
		- Pitch bend?
		- Prepare for VST & finish documentation.
		- Turn structures into real classes piece by piece
		- Expose end filter selection as a parameter
		- The worst crackle and pop is gone but I'm not convinced yet
		  + https://github.com/abaga129/lib_dsp/blob/master/oscillators/GordonSmithOscillator.cpp

	MIDI issues:
		- Smooth MIDI potmeters & wheels!

	Other tasks: 
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- Optional: sample & hold noise
		- After VST: improve oscillators (blend, but only where it looks necessary)
		- After VST: relate GetCarrierHarmonics() to input frequency, now it's just a number that "works"

	Plumbing:
		- Move MIDI calls out of FM. BISON, expose parameters through an object (part of preparation for VST)
		- Stash all oscillators in LUTs, makes it easier to switch or even blend between them and employ the same sampler quality
		- Move all math needed from Std3DMath to synth-math.h, generally stop depending on Bevacqua as a whole

	Of later concern:
		- Find hotspots (plenty!) and optimize (use Jelle v/d Beek's tool)
		- Eliminate floating point values where they do not belong and kill NAN-style bugs (major nuisance, causes crackle!)

	Bugs:
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

	// Trigger a voice (if possible) and return it's voice index
	unsigned TriggerVoice(Waveform form, float frequency, float velocity);

	// Release a note using it's voice index
	void ReleaseVoice(unsigned index);
}

#endif // _FM_BISON_H_
