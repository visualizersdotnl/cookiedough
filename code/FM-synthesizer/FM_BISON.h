

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

	Notes:
		- Code started out with firmware in mind but later on I had to focus on the sound 100% and thus it became a little
		  heavy on the CPU and uses more and more C++
		- Some calculations in here are what is referred to as "bro science"
		- The design is, not on purpose, a bit like the mini MOOG
		- To be optimized

	MiniMOOG design: https://2.bp.blogspot.com/-RRXuwRC_EkQ/WdO_ZKs1AJI/AAAAAAALMnw/nYf5AlmjevQ1AirnXidFJCeNkomYxdt9QCLcBGAs/s1600/0.jpg

	Immediate problems:
		- Implement MinBLEP() for: pulse, hard sync. (priority)
		- Is your approach to FM even right: read book

	Priority tasks:
		- Update parameters multiple times per render cycle
		- Investigate and implement operator feedback
		- Reinstate pitch bend
		- Create interface and stash synthezier into an object
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- Finish documentation as far as possible, read up on VST

	Plumbing:
		- Move shared MIDI code to Win-Midi-in.cpp/h
		- Flush ring buffer using two memcpy() calls
		- Review mutex & atomic use (it's excessive now)
		- Move project to it's own repository (go for VST directly)
		- Consider voice lists
		- Turn structures into real classes piece by piece
		- Move all math needed from Std3DMath to synth-math.h; stop depending on Bevacqua as a whole
		- Tweak velocity & aftertouch (it sounds OK now)

	Research:
		- If bitcrushing is worth having
		- Better wavetable sampling
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
#include "synth-oscillators.h"
#include "synth-voice.h"

bool Syntherklaas_Create();
void Syntherklaas_Destroy();

// Returns loudest voice (linear amplitude)
float Syntherklaas_Render(uint32_t *pDest, float time, float delta);

namespace SFM
{
	/*
		API exposed to (MIDI) input.
	*/

	void TriggerVoice(unsigned *pIndex /* Will receive index to use with ReleaseVoice() */, Waveform form, float frequency, float velocity);
	void ReleaseVoice(unsigned index, float velocity /* Aftertouch */);

	/*
		New API (WIP) for compatibility with VST.

		All parameters are normalized [0..1] unless specified otherwise.
	*/

	class FM_BISON
	{
	public:
		FM_BISON() {}
		~FM_BISON() {}
	};
}
#endif // _FM_BISON_H_


