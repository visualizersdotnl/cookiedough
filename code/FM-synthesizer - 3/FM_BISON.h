

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	** 26/02/2019: wanted to ditch but didn't, going VST, need better performance **

	Third prototype of FM synthesizer
	To be released as VST by Tasty Chips Electronics

	Code is *very* verbose and not optimized at all; still in R&D stage

	Third party credits (not necessarily 100% complete):
		- Bits and pieces taken from Hexter by Sean Bolton (https://github.com/smbolton/hexter)
		- ADSR (modified), original by Nigel Redmon (earlevel.com)
		- JSON++ (https://github.com/hjiang/jsonxx)
		- SvfLinearTrapOptimised2.hpp by FAC (https://github.com/FredAntonCorvest/Common-DSP)

	Core goals:
		- DX7-like core FM
		- Subtractive synthesis on top

	Doing now: 
		- OPL 2-operator mode: research feed forward
		- Create interface on FM_BISON that is called by the MIDI driver(s) insteaed of the other way around
		  + Beware of domains here (dB, time et cetera)
		- Compile WaveSabre and evaluate if that's a good platform for you to continue your work

	To do (in no particular order): 
		- Figure out simple calculation to use power of 2 size delay lines
		- Migrate to VST (WaveSabre?)
		- Pitch envelope needs time stretch
		- Better key scaling implementation (configurable range)
		- Add additive & non-linear level scaling (default is now subtractive & linear)
		- Let all oscillators use table and simplify oscillator logic (split cosine & LFO?)
		- Move algorithms (however crude) to a dedicated file
		- Patch save & load (can be to and from a single file for starters)
		- Try a different form of voice allocation so that a voice can be reused before NOTE_OFF
		- SVF filter goes out of bounds every now and then
		- FIXMEs

	Optimizations:
		- Profile (28/02: doesn't show anything alarming yet, maybe use OMP?)
		- Eliminate expensive floating point functions, look at: https://github.com/logicomacorp/WaveSabre/blob/master/WaveSabreCore/src/Helpers.cpp
		- Optimize fillter
		- Evaluate use of SIMD, but only after a fierce optimization pass (profiler),
		  there's no doubt it will complicate code and make it harder to modify and/or add features

	To do in VST phase:
		- Add initial delay to envelopes (DADSR)
 
	Issues:
		- Oxygen 49 MIDI driver hangs notes every now and then; not really worth looking into
*/

#pragma once

#include "synth-global.h"
#include "synth-stateless-oscillators.h"

bool Syntherklaas_Create();
void Syntherklaas_Destroy();
void Syntherklaas_Render(uint32_t *pDest, float time, float delta);

namespace SFM
{
	/*
		API exposed to (MIDI) input.
		I'm assuming all TriggerVoice() and ReleaseVoice() calls will be made from the same thread, or at least not concurrently.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, unsigned key, float velocity);
	void ReleaseVoice(unsigned index, float velocity /* Aftertouch */);
}
 