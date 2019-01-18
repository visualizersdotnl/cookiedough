

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

	Look at later (TM):
		- Key rate scaling
		  + Primitive implementation: need to define key range to sensibly map linear or non-linear response
		- Level scaling is subtractive and linear only, though this seems to do the job just fine and is less confusing
		  to end users
		- Enhance chorus
		- Revisit string pluck
		- Take some time to read up on how the main filter works

	Optimizations:
		- Use tables for all oscillators
		- Eliminate branches and needless oscillators
		  + A lot of branches can be eliminated by using mask values, which in turns opens us up
		    to possible SIMD optimization
		- Profile and solve hotspots (lots of floating point function calls, to name one)

	Priority: 
		- Finish 'pickup' mode
		  + Define workable user parameter range
		  + Good reason to also look at the integration of different algorithms; and call ones with a zero-frequency collection carrier "wave shaper"
		- Working on integrating filter
		  + Envelope (at least on attack)
		  + Execute per voice
		  + I may need to use an envelope to shape it?
		- Figure out how to interpret aftertouch in ADSR
		- Figure out proper pitch envelope strategy
		- Patch save & load

	Missing top-level features:
		- Jitter
		  + Partially implemented
		- Unison mode?
		  + I'd suggest perhaps per 4 voices (limiting the polyphony)

	A few good tips for better liveliness and Rhodes:
		- https://www.youtube.com/watch?v=72WoueTI354
		- http://recherche.ircam.fr/anasyn/falaize/applis/rhodes/
		- http://www.dafx17.eca.ed.ac.uk/papers/DAFx17_paper_79.pdf

	Golden rules:
		- Basic FM right first, party tricks second: consider going full VST when basic FM works right

	Issues:
		- Feedback depth(s)
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
