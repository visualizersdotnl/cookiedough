
/*
	Syntherklaas FM -- Carrier wave.
*/

#pragma once

#include "synth-global.h"
#include "synth-carrier.h"

namespace SFM
{
	void Carrier::Initialize(unsigned sampleCount, Waveform form, float amplitude, float frequency)
	{
		m_sampleOffs = sampleCount;
		m_amplitude = amplitude;
		m_frequency = frequency;
	
		switch (form)
		{
		case kKick808:
			m_cycleLen = getOscKick808().GetLength();
			break;

		case kSnare808:
			m_cycleLen = getOscSnare808().GetLength();
			break;

		case kGuitar:
			m_cycleLen = getOscGuitar().GetLength();
			break;

		case kElectricPiano:
			m_cycleLen = getOscElecPiano().GetLength();
			break;
			
		default:
			m_cycleLen = kSampleRate/m_frequency;
			break;
		}

		m_oscillator = Oscillator(sampleCount, form, frequency);
	}

	float Carrier::Sample(unsigned sampleCount, float modulation, float pulseWidth)
	{
		const float signal = m_amplitude * m_oscillator.Sample(sampleCount, modulation, pulseWidth);
	
		// It is potentially OK to go beyond, we're mixing amd clamping down the line anyway
//		SampleAssert(signal);

		// So, we do:
		SFM_ASSERT(true == FloatCheck(signal));

		return signal;
	}
}
