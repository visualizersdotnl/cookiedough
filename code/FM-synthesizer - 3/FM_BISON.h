

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
		  + Am I making a mistake by limiting to seconds in my envelopes?
		- Extra depth for modulators

	Discuss with Pieter:
		- Chorus sounds good in 1 particular setup, go old school and offer ON/OFF?

	Priority:
		- Level scaling
		- LFO (behind wheel), sensitivity per operator for amplitude (tremolo) and pitch (vibrato), key sync.?
		- Figure out proper pitch envelope strategy
		- Patch save & load

	Missing top-level features:
		- Filters
		- Vowel filter
		- Jitter

	Golden rule:
		- Basic FM right first, party tricks second

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
