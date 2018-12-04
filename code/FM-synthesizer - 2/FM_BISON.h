

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Second prototype of FM synthesizer
	To be released as VST by Tasty Chips Electronics

	Features (list may be incomplete):
	- 6 programmable operators with:
	  + Tremolo & vibrato LFO (both velocity sensitive, vibrato key scaled)
	  + Yamaha-style level scaling
	  + Amplitude envelope (velocity sensitive, per operator modulation)
	- Global:
	  + Drive
	  + ADSR
	  + Clean & MOOG-style ladder filter (24dB)
	  + Pitch bend
	  + 24 voices (paraphonic)
	  + Adjustable tone jitter (for analogue warmth)
	  + Delay effect with feedback
	- Multiple established FM algorithms

	Goal: reasonably efficient and not as complicated (to grasp) as the real deal (e.g. FM8, DX7, Volca FM)
	
	At first this code was written with a smaller embedded target in mind, so it's a bit of a mixed
	bag of language use at the moment.


	Third party credits:
		- Transistor ladder filter impl. by Teemu Voipio (KVR forum)
		- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
		- ADSR implementation by Nigel Redmon of earlevel.com (fixed and adjusted)

	Things I've figured I should do:
		- Test untested changes, and implement input for:
		  + Velocity sensitivity (amplitude/depth, operator amplitude env.)
		  + Pitch envelope amount
		  + Envelope release scale (ADSR)
		  + Prelim. level scaling (check http://downloads.arturia.com/products/dx7-v/manual/dx7-v_Manual_1_0_EN.pdf)
		- Optimize delay line (see impl.)
		- Run the Visual Studio profiler to locate hotspots for optimization instead of going by
		  "obvious"

	Useful insights:
		- Fixed ratio operators are useful for percussive sounds and such, so support them (working on it, almost done!)
		- The Volca FM sounds very weak when using only 1 operator out of an existing algorithm, so that is how volume is controlled

	Missing (important) features that DX7 and Volca FM have:
		- Stereo
		- My envelopes are different than the ones used by the Volca or DX7, though I'd argue that mine
		  are just as good, *but* I need sharper control for attacks!
		- However: I do not have a full envelope per operator, but a simple AD envelope without R (release),
		  and implied S (sustain); I can fix this when going to VST
		- I only allow velocity sensitivity to be tweaked for operator amplitude and envelope ('velSens'),
		  not pitch env. (always fully responds to velocity)
		- Maybe look into non-synchronized LFOs & oscillators (now they are all key synchronized)
		- Per operator transpose?
		- I lack algorithms

	Also:
		- Check parameter ranges
		- Review operator loop
		- FIXMEs

	Things that are missing or broken:
		- "Clean" filter plops when cutoff is pulled shut: why?
		- Mod. wheel should respond during note playback
		- Potmeters crackle; I see no point in fixing this before I go for VST
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

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, float frequency, float velocity);
	void ReleaseVoice(unsigned index, float velocity /* Aftertouch */);
}
