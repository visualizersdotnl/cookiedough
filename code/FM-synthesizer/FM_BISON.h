

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Beta testers for the VST:
		- Ronny Pries
		- Esa Ruoho
		- Maarten van Strien
		- Mark Smith
		- Bernd H.

	Third-party / References:
		- Transistor ladder filter impl. by Teemu Voipio (KVR forum)
		- Butterworth filter from http://www.musicdsp.org (see header file for details)
		- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
		- ADSR implementation by Nigel Redmon of earlevel.com
		- Basic formant filter by alex@smartelectronix.com (http://www.musicdsp.org)

		And a few more bits and pieces; all credited in the code.

	Notes:
		- This code is a product of ever changing knowledge & goals, and thus a bit inconsistent left and right
		- Some calculations in here are what is referred to as "bro science"
		- To be optimized

	MiniMOOG design: https://2.bp.blogspot.com/-RRXuwRC_EkQ/WdO_ZKs1AJI/AAAAAAALMnw/nYf5AlmjevQ1AirnXidFJCeNkomYxdt9QCLcBGAs/s1600/0.jpg

	Extra priority tasks:
		- Fix bitcrusher, it sounds stupid now
		- Cascade pitch bend?
		- Create operator class and turn voice into an algorithm (eliminate SFM::Modulator?)
		- The battle against clipping chords (in the works)
		- Idea for note drift: Jan Marguc says to add blurred noise to the modulator

	Priority task:
		- Precalculate wavetables for all oscillators and use them
		  + Step 1: precalculate each oscillator at base frequency
		  + Step 2: Mmke oscillator (VCO) use them

	R&D tasks:
		- Consider fine-tuning FM ratio
		- Formant shaping is *very* basic, a few ideas:
		  + Here's some fine information to refine the technique (requires bandpass): https://www.soundonsound.com/techniques/formant-synthesis
		    + Create these using a (cheap-ish) bandpass filter?
		- Update parameters multiple times per render cycle (eliminate all rogue MIDI parameter calls)
		- Implement "sample & hold"
		- Create interface and stash synthesizer into an object
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- First draft of manual

	Plumbing:
		- Make use of smoothed controls in MIDI (gradually as needed)
		- Flush ring buffer using 2 memcpy() calls
		- See if all global state needs to be global
		- Move all math needed from Std3DMath to synth-math.h; stop depending on Bevacqua as a whole
		- Tweak velocity & aftertouch (it sounds OK now)

	Research:
		- I've heard synthesizers getting "brighter" as you hit the keys harder
		- Read about filters a bit more!
		- MinBLEP
		- Monophonic mode (with legato, staccato, glissando et cetera)
		- Real variable delay line
		- Profiling & optimization

	Known bugs:
		- Sometimes an ADSR seems to hang up a voice; or is it a race-condition on NOTE_OFF?
		- MIDI pots crackle a bit (not important for intended target, but can be fixed with MIDI_Smoothed!)
		- Crackle when bottlenecked (should not be the case in production phase)

	Lesson(s) learned:
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


