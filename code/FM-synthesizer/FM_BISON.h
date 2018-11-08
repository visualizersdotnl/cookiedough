

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Beta testers for the VST:
		- Ronny Pries
		- Esa Ruoho
		- Maarten van Strien
		- Mark Smith

	Third-party code used:
		- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
		- Transistor ladder filter impl. by Teemu Voipio (KVR forum)
		- ADSR implementation by Nigel Redmon of earlevel.com
		- MinBLEP table generator by Daniel Werner

	Notes:
		- This code is a product of ever changing knowledge & goals, and thus a bit inconsistent left and right
		- Some calculations in here are what is referred to as "bro science"
		- To be optimized

	MiniMOOG design: https://2.bp.blogspot.com/-RRXuwRC_EkQ/WdO_ZKs1AJI/AAAAAAALMnw/nYf5AlmjevQ1AirnXidFJCeNkomYxdt9QCLcBGAs/s1600/0.jpg

	Immediate problems:
		- Idea: pulse wave sample & hold LFO
		- MiniMOOG mode:
		  + Main carrier should get some prevalence in the mix, if wanted (third knob?)
		- MinBLEP?

	Priority tasks:
		- Update parameters multiple times per render cycle (eliminate all rogue MIDI parameter calls)
		- Investigate and implement operator feedback
		- Reinstate pitch bend
		- Create interface and stash synthesizer into an object
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- First draft of manual

	Plumbing:
		- Move shared MIDI code to Win-Midi-in.cpp/h
		- Flush ring buffer using two memcpy() calls
		- Move project to it's own repository (go for VST directly)
		- Consider voice lists
		- Turn structures into real classes piece by piece
		  + Certain structures, as things mature, look more and more alike: boil it down to the essence
		- Move all math needed from Std3DMath to synth-math.h; stop depending on Bevacqua as a whole
		- Tweak velocity & aftertouch (it sounds OK now)
		  + I've heard synthesizers getting "brighter" as you hit the keys harder

	Research:
		- Monophonic mode (with legato, staccato, glissando et cetera)
		- Real variable delay line
		- Profiling & optimization

	Known issues:
		- MIDI pots crackle a bit (not important for intended target)
		- Numerical instability
		- Crackle when bottlenecked (should not be the case in production phase)

	Lesson(s) learned:
		- Now that I know way more, I should sit down and think of a better design for next time; this has been a proper schooling
		- It is important to follow through and finish this
		- Don't keep pushing a feature that's just not working
*/

#ifndef _FM_BISON_H_
#define _FM_BISON_H_

#include "synth-global.h"
#include "synth-stateless-oscillators.h"
#include "synth-voice.h"

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
#endif // _FM_BISON_H_


