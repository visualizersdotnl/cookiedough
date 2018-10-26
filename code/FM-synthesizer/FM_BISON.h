

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Third-party code used:
		- Magnus Jonsson's Microtracker MOOG filter
		- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
		- ADSR implementation by Nigel Redmon of earlevel.com
		- juce_LinearSmoothedValues.h taken from the Juce library by ROLI Ltd. (only used for test MIDI driver)

	Notes:
		- The code style started out as semi-C, intending a hardware target, currently in the process of cleaning it up a bit
		  and upgrading to somewhat proper C++.
		- You'll run into various numbers and seemingly weird calculations: it's called "bro science".
		- On FM ratios: http://noyzelab.blogspot.com/2016/04/farey-sequence-tables-for-fm-synthesis.html


	Things to do whilst not motivated (read: not hypomanic or medicated):
		- Not much, play a little, don't push yourself

	New ideas / Questions:
		- Switch between 1-3 algorithms

	Priority:
		- BLIT triangle
		- Feedback bug (seems to overload at some point, for some reason, may be the filter)
		- Finish interface and let MIDI driver push all in it's own update loop
		- Turn structures into real classes piece by piece
		- Prepare for VST & finish documentation

	MIDI issues:

	Other tasks: 
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- Optional: sample & hold noise
		- After VST: improve oscillators (blend, but only where it looks necessary)
		- Almost all crackle and pop is gone but I'm not convinced yet

	Plumbing:
		- Floating point bugs
		- Stash all oscillators in LUTs, enables easy tricks and speeds it up (in some cases at least)
		- Move all math needed from Std3DMath to synth-math.h, generally stop depending on Bevacqua as a whole

	Of later concern / Ideas:
		- Scale 808 sample lengths to power of 2
		- Implement real variable delay line
		- Investigate use of the 2 noise oscillators
		- Find hotspots (plenty!) and optimize (use Jelle v/d Beek's tool)
*/

#ifndef _FM_BISON_H_
#define _FM_BISON_H_

#include "synth-global.h"
#include "synth-oscillators.h"

bool Syntherklaas_Create();
void Syntherklaas_Destroy();

// Returns loudest voice (linear amplitude)
float Syntherklaas_Render(uint32_t *pDest, float time, float delta);

namespace SFM
{
	/*
		API exposed to (MIDI) input.
	*/

	// Trigger a voice (if possible) and return it's voice index
	unsigned TriggerVoice(Waveform form, float frequency, float velocity);

	// Release a note using it's voice index
	void ReleaseVoice(unsigned index);

	/*
		New API (WIP) for compatibility with VST.

		All parameters are normalized [0..1] unless specified otherwise.
	*/

	class FM_BISON
	{
	public:
		FM_BISON() {}
		~FM_BISON() {}

		/*
			Voice API.
		*/

		// Trigger a voice (if possible) and return it's voice index
		unsigned TriggerVoice(Waveform form, float frequency, float velocity);

		// Release a note using it's voice index
		void ReleaseVoice(unsigned index);

		/*
			Parameters.
		*/

		// Master drive
		void SetMasterDrive(float drive);

		// Pitch bend
		void SetPitchBend(float value);

		// Modulation
		void SetModulationIndex(float value);
		void SetModulationRatio(float value); // Indexes a 15-deep 'Farey Sequence' C:M table
		void SetModulationBrightness(float value);
		void SetModulationLFOFrequency(float value);
		
		// Filter
		void SetFilterCutoff(float value);
		void SetFilterResonance(float value);
		void SetFilterWetness(float value);
		void SetFilterEnvelopeInfluence(float value); // Filter envelope is a tweaked copy of the current ADSR

		// Feedback
		void SetFeedback(float value);
		void SetFeedbackWetness(float value);
		void SetFeedbackPitch(float value);

		// ADSR (also used, automatically altered, by filter)
		void SetAttack(float value);
		void SetDecay(float value);
		void SetSustain(float value);
		void SetRelease(float value);

		// Tremolo
		void SetTremolo(float value);
	};
}

#endif // _FM_BISON_H_
