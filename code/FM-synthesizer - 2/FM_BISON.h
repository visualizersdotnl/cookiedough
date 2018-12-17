

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Second prototype of FM synthesizer
	To be released as VST by Tasty Chips Electronics

	Features (list may be incomplete or incorrect at this stage):
	- 6 programmable operators with:
	  + Tremolo & vibrato LFO (vibrato key scaled)
	  + Yamaha-style level scaling
	  + Amplitude envelope (velocity sensitive, per operator modulation)
	- Global:
	  + Drive, ADSR, feedback amount, tremolo & vibrato speed
	  + Pitch bend, global modulation
	  + Clean & MOOG-style ladder filter (24dB) with envelope
	  + Multiple voices (paraphonic)
	  + Adjustable tone jitter (for analogue warmth)
	  + Configurable delay effect with optional feedback
	  + Per-voice calculated "rough" vowel filter
	- Multiple established FM algorithms

	Goal: reasonably efficient and not as complicated (to grasp) as the real deal (e.g. FM8, DX7, Volca FM)
	
	At first this code was written with a smaller embedded target in mind, so it's a bit of a mixed
	bag of language use at the moment.


	Third party credits:
		- Different lowpass filters: see synth-filter.h
		- ADSR implementation by Nigel Redmon of earlevel.com (fixed and modified, perhaps I should share)
		- Some parts and information taken from: https://github.com/smbolton/hexter/blob/master/src/
		- Vowel filter adapted from a post by alex@smartelectronix.com @ http://www.musicdsp.org

	Problems I encountered:
		- If I switch octave on the Oxygen, the level scaling breakpoints should move along with it?
		- I feel that adding more of my oscillator types would be helpful?

	VST version must-haves:
		- Full envelopes everywhere!
		- Use modulation wheel for what it's supposed to be for, not your global "OSC Rate Scale" (see Volca FM sheet)

	Working on:
		- Make vowel shaping amount depend on velocity (for that pickup effect)
		  + Should this be configurable? Maybe just under a button as "Rhodey-mode"?
		  + Larger (bass range) tines are more susceptible to this than the others, do I want to go that far?
		- Voice stealing
		  + Implement theft of releasing voices in *sorted* order (implemented, test!)
		- Amplitude/Depth ratio (see Hexter tab.)
		- I need a decent way to define algorithms; hardcoding them like I do now is too error prone

	Sound improvements:
		- Use sustain pedal to ignore NOTE_OFF
		- Improve delay LFO frequency, it's beating too much now
		- Evaluate the idea of global LFOs that are *not* synchronized
		  + Are they any better than randomizing?
		- Rip (more of) Sean Bolton's tables
		  + See: https://github.com/smbolton/hexter/blob/master/src/dx7_voice_tables.c
		- Review op. env. AD
		- Finish level scaling
		  + Configurable range
		  + LIN/EXP choice
		  + More potential depth?
		- Optimize delay line (see impl.)
		- Check vowel filter: can I limit the input instead of clamping the output?

	Two of Pieter's ideas:
		- Take out feedback
		- And I suggest on top of that: 1 modulator only, not 3

	Missing (important) features that DX7 and Volca FM have:
		- Stereo
		- My envelopes are different than the ones used by the Volca or DX7, though I'd argue that mine
		  are just as good, *but* I need sharper control for attacks!
		- However: I do not have a full envelope per operator, but a simple AD envelope without R (release),
		  and implied S (sustain); I can fix this when going to VST
		- I only allow velocity sensitivity to be tweaked for operator amplitude and envelope ('velSens'),
		  not pitch env. (always fully responds to velocity)
		- Maybe look into non-synchronized LFOs & oscillators (now they are all key synchronized)
		- I lack algorithms, but that'll be fixed down the line
		- There are more differences but don't forget you're *not* making a DX7

	Don't forget to:
		- Check parameter ranges
		- Review operator loop
		- FIXMEs

	Things that are missing or broken:
		- Mod. wheel should respond during note playback
		- Potmeters crackle; I see no point in fixing this before I go for VST

	Jan Marguc says:
		- Try wavetables as modulator
		- More decay curve sens.
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
