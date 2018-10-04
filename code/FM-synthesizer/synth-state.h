

/*
	Syntherklaas -- FM synthesizer prototype.
	Global state PODs.
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
		unsigned m_sample;

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
		unsigned m_sample;
		unsigned m_numHarmonics;

		void Initialize(Waveform form, float amplitude, float frequency);
		float Sample(float modulation);
	};

	/*
		Voice.
	*/

	struct FM_Voice
	{
		bool enabled;

		FM_Carrier carrier;
		FM_Modulator modulator;

		float Sample()
		{
			// FIXME: simplest algorithm there is, expand!
			float modulation = modulator.Sample(nullptr);
			return carrier.Sample(modulation);
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
