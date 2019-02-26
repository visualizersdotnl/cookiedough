

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	** 26/02/2019: project ditched, going VST **

	Third prototype of FM synthesizer
	To be released as VST by Tasty Chips Electronics

	Code is *very* verbose and not optimized at all; still in R&D stage

	Third party credits (not necessarily 100% complete):
		- Bits and pieces taken from Hexter by Sean Bolton (https://github.com/smbolton/hexter)
		- ADSR (modified), original by Nigel Redmon (earlevel.com)
		- Pink noise function by Paul Kellet (http://www.firstpr.com.au/dsp/pink-noise/)
		- JSON++ (https://github.com/hjiang/jsonxx)
		- SvfLinearTrapOptimised2.hpp by FAC (https://github.com/FredAntonCorvest/Common-DSP)

	Core goals:
		- DX7-like core FM
		- Subtractive synthesis on top

	To do (in no particular order):
		- Move synth-util.h and synth-math.h into synth.helper.h
		- Pitch envelope needs time stretch like the others do
		- Add initial delay to envelopes (DADSR)
		- Migrate to VST
		- Better key scaling implementation (configurable range)
		- Add additive & non-linear level scaling (default is now subtractive & linear)
		- Optimize fillter
		- Use smaller delay line(s) with chorus
		- Let all oscillators use table and simplify oscillator ogic (split normal & LFO?)
		- Eliminate expensive floating point functions, look at: https://github.com/logicomacorp/WaveSabre/blob/master/WaveSabreCore/src/Helpers.cpp
		- Evaluate use of SIMD, but only after a fierce optimization pass (profiler)
		- Move algorithms (however crude) to a dedicated file
		- Patch save & load (can be to and from a single file for starters)
		- Create interface on FM_BISON that is called by the MIDI driver(s) insteaed of the other way around
		  + Beware of domains here (dB, time et cetera)
		- Is the SVF filter stable and if not is it becauase if cast back to float?
		- My delay line is clunky and does not resize
		- Implement blend between flat and curvy ADSR
		- Try a different form of voice allocation so that a voice can be reused before NOTE_OFF
		- FIXMEs

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
