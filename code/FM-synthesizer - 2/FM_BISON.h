

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	This is my second attempt at an FM synthesizer:
	- 6 programmable operators with envelope and shared tremolo & vibrato LFOs
	- Master drive & ADSR
	- Master clean 24dB & MOOG 24dB ladder filters
	- Global modulation control & pitch envelope (AD)
	- Pitch bend
	- Multiple tried & tested FM algorithms
	- Paraphonic (24 voices)
	- Adjustable tone jitter (for analogue warmth)
	- Tuneable global delay effect

	Until this feature set works and is reasonably efficient, no other features are to be added.
	At this point this code is not optimized for speed but there's tons of low hanging fruit.

	Third party credits:
	- Transistor ladder filter impl. by Teemu Voipio (KVR forum)
	- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
	- ADSR implementation by Nigel Redmon of earlevel.com (fixed and adjusted)

	To learn during vacation:
		- Actual device-based FM programming, take copious notes!
		- Look for optimizations
		- I *need* level scaling: https://www.youtube.com/watch?v=0XY2IcwNVnk
		  + Je geeft aan wat het midden van je toetsenbord is voor operator N, en aan weerskanten hoe steep en lang de curve is to demodulate!
`
	Useful insights:
		- Fixed ratio operators are useful for percussive sounds and such, so support them (working on it, almost done!)
		- The Volca FM sounds very weak when using only 1 operator out of an existing algorithm, so that is how volume is controlled

	Missing for near-DX7/Volca compatibility:
		- Stereo
		- My envelopes are different than the ones used by the Volca or DX7, though I'd argue that mine
		  are just as good, *but* I need sharper control for attacks!
		- However: I do not have a full envelope per operator, but a simple AD envelope without R (release)
		- I do not support 'level scale' (I want to, but it is not crucial to sounding compatible on note level)
		  + Ask Kilian, see URL above!
		- My operators do not have programmable velocity sensitivy
		- I lack algorithms
	
	Also:
		- Check parameter ranges
		- Review operator loop
		- All FIXMEs
		- Optimization; you're working with powers of 2 and cache misses left and right

	Things that are missing or broken:
		- "Clean" filter plops when cutoff is pulled shut: why?
		- Mod. wheel should respond during note playback
		- Potmeters crackle; I see no point in fixing this before I go for VST
		- Cherry pick from the first iteration's lists
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
