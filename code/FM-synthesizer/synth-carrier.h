
/*
	Syntherklaas FM -- Carrier wave.
*/

#pragma once

#include "synth-oscillators.h"

namespace SFM
{
	struct Carrier
	{
		Waveform m_form;
		float m_amplitude;
		float m_frequency;
		float m_pitch;
		unsigned m_sampleOffs;
		unsigned m_numHarmonics;
		float m_cycleLen; // In samples

		void Initialize(unsigned sampleCount, Waveform form, float amplitude, float frequency);
		float Sample(unsigned sampleCount, float modulation);

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
