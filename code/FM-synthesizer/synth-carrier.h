
/*
	Syntherklaas FM -- Carrier wave.

	Only supports a limited set of waveforms (see implementation).
*/

#pragma once

#include "synth-oscillators.h"

namespace SFM
{
	// Initialized manually
	struct Carrier
	{
		unsigned m_sampleOffs;
		Waveform m_form;
		float m_amplitude;
		float m_frequency;
		float m_pitch;
		float m_cycleLen; // In samples

		void Initialize(unsigned sampleCount, Waveform form, float amplitude, float frequency);
		float Sample(unsigned sampleCount, float modulation, float pulseWidth);

		// To implement one-shot (FIXME: maybe move to a more generic place)
		SFM_INLINE bool HasCycled(unsigned sampleCount) /* const */
		{
			return ceilf((sampleCount-m_sampleOffs)*m_pitch) >= m_cycleLen;
		}

		SFM_INLINE bool IsWavetableForm() /* const */
		{
			return oscIsWavetable(m_form);
		}
	};
}
