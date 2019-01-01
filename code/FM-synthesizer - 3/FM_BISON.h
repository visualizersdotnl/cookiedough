

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Third prototype of FM synthesizer
	To be released as VST by Tasty Chips Electronics

	Third party credits (not necessarily 100% complete):
		- Bits and pieces taken from Hexter by Sean Bolton (https://github.com/smbolton/hexter)
		- ADSR (modified), original by Nigel Redmon (earlevel.com)
		- Pink noise function by Paul Kellet (http://www.firstpr.com.au/dsp/pink-noise/)
		- ...

	Core goals:
		- DX7-like core FM
		- Subtractive synthesis on top

	Look at later (TM):
		- Rate scaling (basically means speeding up (parts of) the operator ADSR)
		  + Am I making a mistake by limiting to seconds in my envelopes? Think so!
		- Modulation & feedback depth(s)
		- Enhance chorus & distortion (or maybe remove the latter)

	Priority:
		- Figure out proper pitch envelope strategy
		- Level scaling
		  + Primitive implementation is done: breakpoint, subtractive/linear, amount L/R, range in semitones
		    This may well be what's needed apart from exponential (maybe!) until I see a point in going additive
		- Patch save & load

	Missing top-level features:
		- Jitter
		  + Partially implemented
		- Filters (LPF, vowel)
		- A unison mode?

	Golden rules:
		- Basic FM right first, party tricks second
		- Optimization is possible nearly everywhere; do not do any before the instrument is nearly done

	Issues:
		- Oxygen 49 MIDI driver hangs notes every now and then; not really worth looking into

	Keep yellow & blue on the Oxygen 49, the subtractive part on the BeatStep.
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

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, unsigned key, float velocity);
	void ReleaseVoice(unsigned index, float velocity /* Aftertouchs */);
}
