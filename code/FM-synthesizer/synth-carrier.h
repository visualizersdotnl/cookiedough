
/*
	Syntherklaas FM -- Carrier wave.
*/

#pragma once

// #include "synth-oscillators.h"
#include "synth-oscillator.h"

namespace SFM
{
	// Initialized manually
	struct Carrier
	{
		unsigned m_sampleOffs;
		float m_amplitude;
		float m_frequency;
		float m_cycleLen;
		Oscillator m_oscillator;

		void Initialize(unsigned sampleCount, Waveform form, float amplitude, float frequency);
		float Sample(unsigned sampleCount, float modulation, float pulseWidth);

		SFM_INLINE bool HasCycled(unsigned sampleCount) /* const */
		{
			return (sampleCount-m_sampleOffs)*m_oscillator.GetPitch() >= m_cycleLen;
		}

		SFM_INLINE bool IsWavetableForm() /* const */
		{
			return oscIsWavetable(m_oscillator.GetWaveform());
		}
	};
}
