

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

	In VST phase:
	    - Pitch envelope: refine, add time stretch!
		- Different LFO waveforms
		- Envelope on main filter
		- Make envelope a 'DADSR' to add an initial delay
		- Better key scaling implementation (configurable range)
		- Add additive & non-linear level scaling (default is now subtractive & linear)
		- Optimize fillter
		- Enhance chorus
		- Machine learning for patches?

	(OPTIONAL) Concerning a super and/or hyper saw:
		- Read: https://www.nada.kth.se/utbildning/grukth/exjobb/rapportlistor/2010/rapporter10/szabo_adam_10131.pdf

	(OPTIONAL) For FM-X support:
		- Add it's other core waveforms
		- Implement 'Skirt' and 'Res' (basically a resonant lowpass); these can be cut to 8 discrete steps!
		- Demo: https://www.youtube.com/watch?v=YWvSglv3iEA
	
	Optimization:
		- All oscillators become tables, oscillator & LFO logic may be split
		- Eliminate experince floating point functions, look at: https://github.com/logicomacorp/WaveSabre/blob/master/WaveSabreCore/src/Helpers.cpp
		- Elimnate branches & needless logic
		  + Filters can be processed sequentially *after* basic voice logic
		  + A lot can be eliminated through the use of masks
		  + SIMD + 8 operators?
		- Use the damn profiler!

	Plumbing:
		- Move algorithms to dedicated file
		- Patch save & load

	Priority plus:
		- Finish Wurlitzer grit 
		  + Filter
		  + S&H
		- Create interface on FM_BISON that is being called by the MIDI driver(s) instead of the other way around
		  + Beware of domains here (dB, time et cetera)
		- Immediately after: implement instrument serializer; for now it can just store and load to/from a single file!
		- The SVF filter goes out of bounds? I suppose this because I cast to float in the end!
		- My delay line is clunky; I could use smaller ones at the cost of some precision (look at WaveSabre)
		- Implement parameter to flatten the ADSR
		- Figure out how to interpret aftertouch in ADSR
		- Try a different form of voice allocation so that a voice can be reused before NOTE_OFF

	Priority:
		- Migrate to VST
		  + Allows to better estimate which paramaters need range adjustment
		  + Right now I'm setting the patch according to MIDI values, but that should not be done that way
		    in a VST harness
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
