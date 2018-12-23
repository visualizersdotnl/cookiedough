

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Second prototype of FM synthesizer
	To be released as VST by Tasty Chips Electronics

	Features as of 23/12/2018:
		- 16 voice polyphony (to be upgraded to 64)
		- 6-operator Yamaha style, including level scaling
		- Per operator envelope (AD), tremolo, vibrato, pitch (sweep)
		- Global ADSR
		- Global 3-phase chorus
		- Per voice low-pass filter (2 switchable MOOG-style variants) with envelope
		- Multiple algorithms
		- Global tone jitter or "life" adjustment for a more natural sound
		- Basic vowel (formant) filter

	Goal: reasonably efficient and not as complicated (to grasp) as the real deal (e.g. FM8, DX7, Volca FM)
	
	At first this code was written with a smaller embedded target in mind, so it's a bit of a mixed
	bag of language use at the moment.

	----
	And as of 23/12/2018 I think we're best off keeping this codebase as a reference for the VST.
	----

	
	Third party credits:
		- Different lowpass filters: see synth-filter.h
		- ADSR implementation by Nigel Redmon of earlevel.com (fixed and modified, perhaps I should share)
		- Some parts and information taken from Sean Bolton's Hexter: https://github.com/smbolton/hexter/blob/master/src/
		- Vowel filter adapted from a post by alex@smartelectronix.com @ http://www.musicdsp.org

	Problems I encountered:
		- If I switch octave on the Oxygen, the level scaling breakpoints should move along with it?

	VST version must-haves:
		- Rethink the entire approach (DX voice render especially)
		- Full envelopes everywhere?
		- ** The jitter mode is CRUCIAL to mask beating! **

	Working on:
		- There's range problems, those Hexter tables need to be looked at  <- * IN PROGRESS *
		- Polish new vibrato
		- Finish level scaling (controls are now available)
		  + Configurable range per side (in octaves, use 2 unused potentiometers on BeatStep)
		  + More potential depth?
		- New delay (chorus) effect needs polishing
		- Algorithms
		- Use logarithmic scale for operator amplitude (carriers at least?)
		- Make vowel shaping amount depend on velocity (for that pickup effect)
		  + It works, but:
		  + Should this be configurable? Maybe just under a button as "Rhodey-mode"?
		- Voice stealing
		  + Implement theft of releasing voices in *sorted* order (implemented, test!)

	Sound improvements:
	    - Optimize ring buffer use
		- Use sustain pedal to ignore NOTE_OFF
		- Tweak LFO jitter ranges
		- Optimize delay line (see impl.)
		- Check vowel filter: can I limit the input instead of clamping the output?
		- Most oscillator types can be taken out and you'd be left with a sinee, cosine and triangle, which ideally you
		  read from a table directly, this will make Oscillator a good bit quicker

	Two of Pieter's ideas:
		- Take out feedback and replace it for a triangle or saw oscillator
		- And I suggest on top of that: 1 modulator only, not 3

	Missing (important) features that DX7 and Volca FM have:
		- My envelopes are different than the ones used by the Volca or DX7 (can be fixed when going to VST)
		- I only allow velocity sensitivity to be tweaked for operator amplitude and envelope ('velSens'),
		  not pitch env. (always fully responds to velocity)
		- Maybe look into non-synchronized LFOs & oscillators (now they are all key synchronized)

	Don't forget:
		- FIXMEs
		- Review all features
		- DX7 patch converter/importer?

	Things that are missing or broken:
		- Potmeters crackle; I see no point in fixing this before I go for VST

	Jan Marguc says:
		- Try wavetables as modulator
		- More decay curve sens.

	Something I stumbled upon late at night:
	
	"The envelopes in the DX7 are quite weird. The decays are fine, but since they're linear envelopes passed through an exponential table, 
	 the attacks end up naturally being a sort of "reverse exponential" that resembles a time-reversed decay rather than a natural attack (in analog ADSRs this is generally an inverted exponential decay, which sounds much better)."
*/

#pragma once

#include "synth-global.h"
#include "synth-stateless-oscillators.h"

bool Syntherklaas_Create();
void Syntherklaas_Destroy();

// Returns loudest voice (linear amplitude)
float Syntherklaas_Render(uint32_t *pDest, float time, float delta);

namespace SFM
{
	/*
		API exposed to (MIDI) input.
		I'm assuming all TriggerVoice() and ReleaseVoice() calls will be made from the same thread, or at least not concurrently.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, unsigned key, float velocity);
	void ReleaseVoice(unsigned index, float velocity /* Aftertouch */);
}
