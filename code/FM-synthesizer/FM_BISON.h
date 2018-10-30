

/*
	'FM. BISON' by syntherklaas.org, a subsidiary of visualizers.nl

	Beta-test list:
		- Ronny Pries
		- Esa Ruoho
		- Maarten van Strien
		- Mark Smith

	Third-party code used:
		- Magnus Jonsson's Microtracker MOOG filter
		- D'Angelo & Valimaki's improved MOOG filter (paper: "An Improved Virtual Analog Model of the Moog Ladder Filter")
		- ADSR implementation by Nigel Redmon of earlevel.com
		- juce_LinearSmoothedValues.h taken from the Juce library by ROLI Ltd. (only used for test MIDI driver)
		- Transistor ladder filter impl. by Teemu Voipio (KVR forum)

	Notes:
		- I started programming with low-level embedded targets in mind; over time I decided to go towards conservative C++11.
		  At the moment you'll see a somewhat mixed bag and it'll probably stay that way.
		- A lot of calculations are based upon tweaking and thus have no real scientific basis; it is called "bro science".
		- The filters can cock up if you're lacking procesisng power at that particular moment.

	Things to do whilst not motivated (read: not hypomanic or medicated):
		- Not much, play a little, don't push yourself

	Ideas:
		- Dirt factor (noise?)
		- Figure out 1 more useful algorithm

	Priority (pre-VST):
		- The PolyBLEP square wave seems to be unstable: investigate!
		- Finish interface and let MIDI driver push all in it's own update loop
		- Prepare for VST & finish documentation
		- Try a 'hold' state to sustain wavetable samples
		- Review mutex usage

	Lower priority: 
		- Voice stealing (see KVR thread: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=91557&sid=fbb06ae34dfe5e582bc8f9f6df8fe728&start=15)
		- Research sample & hold
		- Almost all crackle and pop is gone but I'm not convinced yet

	Plumbing:
		- Floating point bugs (keep adding SampleAssert())
		- Turn structures into real classes piece by piece
		- Move all math needed from Std3DMath to synth-math.h, generally stop depending on Bevacqua as a whole

	Of later concern / Ideas:
		- Implement real variable delay line
		- Investigate use of the 2 noise oscillators (and create blue noise, which is the inverse of pink noise)
		- Think about wrapping the phases, this allows for simpler oscillator algorithms (faster)
		- Resample wavetable voices to power-of-2 size
		- Find hotspots (plenty!) and optimize (use Jelle v/d Beek's tool)
		- Use multiple threads, SIMD

	Known issues:
		- MIDI pots crackle a bit (not important for intended target)
		- Numerical instability
		- Crackle when bottlenecked
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

		// Algorithm
		void SetAlgorithm(Voice::Algorithm algorithm);
		void SetAlgoTweak(float value);

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
		void SetFilterType(int value); // 1 = Improved MOOG ladder, 2 = Microtracker MOOG ladder, 0 (default) = Teemu's transistor ladder
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

		// One-shot 
		void SetCarrierOneShot(bool value);
	};
}

#endif // _FM_BISON_H_
