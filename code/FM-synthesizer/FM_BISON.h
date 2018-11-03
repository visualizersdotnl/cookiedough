

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
		- Code was written directly with firmware in mind, but later on as I found out that this project involved so much new
		  information I went towards a bit of C++ here and there; I also decided to get the sound right and try to understand
		  what I'm doing which has led to some wonky design decisions. Nevertheless I will finish this project.
		- Some calculations in here are what is referred to as "bro science".
		- The design is, not on purpose, a bit like the mini MOOG.
		- No optimization as of yet, not at all.

	MiniMOOG design: https://2.bp.blogspot.com/-RRXuwRC_EkQ/WdO_ZKs1AJI/AAAAAAALMnw/nYf5AlmjevQ1AirnXidFJCeNkomYxdt9QCLcBGAs/s1600/0.jpg

	Priority tasks:
		- Update parameters multiple times per render cycle?
		- Reinstate pitch bend
		- Figure out if MIDI parameters need smoothing at all
		- Finish up interface and let the MIDI driver feed it
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- Finish documentation as far as possible, read up on VST

	Plumbing:
		- Flush ring buffer using two memcpy() calls
		- Review mutex & atomic use
		- Encapsulate the core in a class so there can be instances
		- Normalize wavetables
		- Move project to it's own repository (go for VST directly)
		- Always keep looking (and placing assertions) to trap floating point bugs
		- Consider voice lists
		- Turn structures into real classes piece by piece
		- Move all math needed from Std3DMath to synth-math.h, generally stop depending on Bevacqua as a whole
		- Tweak velocity & aftertouch (it sounds OK now)

	Research:
		- Sample & hold
		- Better wavetable sampling
		- Real variable delay line
		- Profiling & optimization

	Known issues:
		- Filters clip when stopped short (like Frank Costanza does it)
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

		/*
			Voice API.
		*/

		// Trigger a voice (if possible) and return it's voice index
		unsigned TriggerVoice(Waveform form, float frequency, float velocity);

		// Release a note using it's voice index
		void ReleaseVoice(unsigned index, float velocity /* Aftertouch */);

		/*
			Parameters.

			All in [0..1] range unless stated otherwise.
			FIXME: update (keep in mind that at this moment some values are pulled directly from the MIDI drivers)
		*/

		// Algorithm
		void SetAlgorithm(Voice::Algo algorithm);
		// FIXME: ...

		// Master drive
		void SetMasterDrive(float drive);

		// Pitch bend
		void SetPitchBend(float value);

		// Modulation
		void SetModulationIndex(float value);
		void SetModulationRatio(float value); // Indexes a 'Farey Sequence' C:M table
		void SetModulationBrightness(float value);
		void SetModulationLFOFrequency(float value);
		
		// Filter
		void SetFilterType(VoiceFilter value); 
		void SetFilterDrive(float value); // Linear representation of [0..1]*+6dB
 		void SetFilterCutoff(float value);
		void SetFilterResonance(float value);
		void SetFilterContour(float value);
		
		// Feedback
		void SetFeedback(float value);
		void SetFeedbackWetness(float value);
		void SetFeedbackPitch(float value);

		// Voice ADSR
		void SetAttack(float value);
		void SetDecay(float value);
		void SetSustain(float value);
		void SetRelease(float value);

		// Filter ADS
		void SetFilterAttack(float value);
		void SetFilterDecay(float vvalue);
		void SetFilterustain(float value);

		// Tremolo
		void SetTremolo(float value);

		// Noisyness (amount of pink noise added during final mix)
		void SetNoisyness(float value);

		// One-shot 
		void SetCarrierOneShot(bool value);
	};
}

#endif // _FM_BISON_H_
