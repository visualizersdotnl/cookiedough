
/*
	Syntherklaas FM -- Carrier wave.
*/

#pragma once

// #include "synth-oscillators.h"
#include "synth-oscillator.h"

namespace SFM
{
	// FIXME: create class
	struct Carrier
	{
		unsigned m_sampleOffs;
		float m_amplitude;
		Oscillator m_oscillator;

		void Initialize(unsigned sampleCount, Waveform form, float amplitude, float frequency)
		{
			m_sampleOffs = sampleCount;
			m_amplitude = amplitude;
			m_oscillator = Oscillator(sampleCount, form, frequency);
		}

		float Sample(unsigned sampleCount, float modulation, float pulseWidth)
		{
			const float signal = m_amplitude * m_oscillator.Sample(sampleCount, modulation, pulseWidth);
	
			// It is potentially OK to go beyond, we're mixing amd clamping down the line anyway
			SampleAssert(signal);

			return signal;
		}

		SFM_INLINE bool HasCycled(unsigned sampleCount) /* const */
		{
			return (sampleCount-m_sampleOffs)*m_oscillator.GetPitch() >= m_oscillator.GetPeriodLength();
		}
	};
}
