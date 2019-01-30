

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

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

	Look at later (when in VST phase):
		- Feedback is now reversed (sine -> reverse saw)
		- Better key scaling implementation (configurable range)
		- Additive level scaling
		- Enhanced chorus
		- Filter

	Super/Hyper saw:
		- Read: https://www.nada.kth.se/utbildning/grukth/exjobb/rapportlistor/2010/rapporter10/szabo_adam_10131.pdf

	FM-X:
		- Will need per operator waveforms (look at FM-X + the TX81Z)
		- Need 'Skirt' and 'Res' (basically a resonant lowpass) per operator
		  This can be cut down to 8 discrete steps and thus be precalculated (on set, at least) in LUTs
		- I am covering part of this with my overdrive parameter
		- Information: https://www.youtube.com/watch?v=YWvSglv3iEA
	
	Optimizations:
		- Use tables for all oscillators
		- Eliminate branches and needless oscillators
		  + A lot of branches can be eliminated by using mask values, which in turns opens us up
		    to possible SIMD optimization (read up on this, even though you know SIMD inside and out)
		- Profile and solve hotspots (lots of floating point function calls, to name one)

	Plumbing:
		- Try a parameter to flatten or invert the attack curve of the ADSR?
		- Move algorithms to dedicated file
		- Patch save & load

	Priority:
		- Try completely different instruments
		- Migrate to VST!
		  + Also allows for a better estimate which parameters need range adjustment
		- Finish 'pickup' mode (at the very least specify a safe parameter range)
		  + Blend knob implemented, but I feel it's not realy necessary
		- DADSR? (or: add delay prior to attack)
		- Figure out how to interpret aftertouch in ADSR
		- Figure out proper pitch envelope strategy
		- Look at recent FIXMEs, rethink your life, read http://people.ece.cornell.edu/land/courses/ece4760/Math/GCC644/FM_synth/Chowning.pdf

	Missing top-level features:
		- Jitter is only partially implemented
		- Unison mode, portamento (monophonic) perhaps

	A few good tips for better liveliness and Rhodes: 
		- https://www.youtube.com/watch?v=72WoueTI354
		- http://recherche.ircam.fr/anasyn/falaize/applis/rhodes/
		- http://www.dafx17.eca.ed.ac.uk/papers/DAFx17_paper_79.pdf

	Golden rules:
		- Basic FM right first, party tricks second: consider going full VST when basic FM works right
		- Don't chase the DX7: you'll be spending hours emulating algorithms based on past restrictions (reading doesn't hurt, though)

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

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, unsigned key, float velocity);
	void ReleaseVoice(unsigned index, float velocity /* Aftertouchs */);
}
