
/*
	Syntherklaas FM -- Global state PODs.

	** Important note: since everything is copied for now, you can't have persistent state **
*/

#ifndef _SFM_SYNTH_STATE_H_
#define _SFM_SYNTH_STATE_H_

#include "synth-global.h"
#include "synth-oscillators.h"

namespace SFM
{
	/*
		FM modulator.
	*/

	struct FM_Modulator
	{
		float m_index;
		float m_pitch;
		unsigned m_sampleOffs;

		void Initialize(float index, float frequency);
		float Sample(const float *pEnv);
	};

	/*
		FM carrier.
	*/

	struct FM_Carrier
	{
		Waveform m_form;
		float m_amplitude;
		float m_pitch;
		unsigned m_sampleOffs;
		unsigned m_numHarmonics;

		void Initialize(Waveform form, float amplitude, float frequency);
		float Sample(float modulation);
	};

	/*
		ADSR envelope.
	*/

	struct ADSR
	{
		unsigned m_sampleOffs;

		// In number of samples
		unsigned m_attack;
		unsigned m_decay;
		unsigned m_release;

		// Desired sustain [0..1]
		float m_sustain;

		enum State
		{
			kAttack,
			kDecay,
			kSustain,
			kRelease
		} m_state;

		void Start(unsigned sampleOffs);
		void Stop(unsigned sampleOffs);
		float Sample(); // Sets 'enabled' to false if voice is released by ADSR.
	};

	/*
		Voice.
	*/

	struct FM_Voice
	{
		bool enabled;

		FM_Carrier carrier;
		FM_Modulator modulator;
		ADSR envelope;

		// FIXME: idea: pass global envelope here, like Ronny said, along with operation!
		float Sample()
		{
			// FIXME: simplest algorithm there is, expand!
			const float modulation = modulator.Sample(nullptr);
			const float ampEnv = envelope.Sample();
			const float sample = carrier.Sample(modulation)*ampEnv;
			return sample;
		}
	};

	/*
		Global state.
	*/

	struct FM
	{
		FM_Voice voices[kMaxVoices];

		void Reset()
		{
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				voices[iVoice].enabled = false;
			}
		}

	};
}

#endif // _SFM_SYNTH_STATE_H_
