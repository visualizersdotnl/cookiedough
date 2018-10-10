
/*
	Syntherklaas FM -- Global (state) PODs.

	Everything is copied per render cycle to a 'live' state; because of this it is important
	*not* to have any state altered during rendering as it will be lost.
*/

#ifndef _SFM_SYNTH_STATE_H_
#define _SFM_SYNTH_STATE_H_

#include "synth-global.h"
#include "synth-modulator.h"
#include "synth-vorticity.h"

namespace SFM
{
	/*
		FM carrier.
	*/

	struct FM_Carrier
	{
		Waveform m_form;
		float m_amplitude;
		float m_pitch, m_angularPitch;
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
		float Sample();	
	};

	/*
		Voice.
	*/

	struct FM_Voice
	{
		bool m_enabled;

		FM_Carrier m_carrier;
		FM_Modulator m_modulator;
		ADSR m_envelope;
		Vorticity m_vorticity;

		// FIXME: idea: pass global envelope here, like Ronny said, along with operation!
		// FIXME: simplest algorithm there is, expand!
		float Sample()
		{
			const float modulation = m_modulator.Sample(nullptr);
			const float ampEnv = m_envelope.Sample();
			float sample = m_carrier.Sample(modulation)*ampEnv;

			if (m_envelope.m_state == ADSR::kRelease)
			{
				const float vorticity = m_vorticity.Sample();
				sample *= vorticity;
			}

			return sample;
		}
	};

	/*
		Complete state.
	*/

	struct FM
	{
		FM_Voice m_voices[kMaxVoices];
		unsigned m_active;

		void Reset()
		{
			for (unsigned iVoice = 0; iVoice < kMaxVoices; ++iVoice)
			{
				m_voices[iVoice].m_enabled = false;
			}

			m_active = 0;
		}

	};
}

#endif // _SFM_SYNTH_STATE_H_
