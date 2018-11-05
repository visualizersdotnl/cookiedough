
/*
	Syntherklaas FM -- Simple continuous oscillator (LFO).
*/

#pragma once

#include "synth-global.h"
#include "synth-LFO.h"

namespace SFM
{
	void LFO::Initialize(unsigned sampleCount, Waveform form, float depth, float frequency)
	{
		SFM_ASSERT(frequency <= kMaxSonicLFO);

		m_sampleOffs = sampleCount;
		m_form = form;
		m_depth = depth;
		m_frequency = frequency;
		m_pitch = CalculatePitch(frequency);

		m_enabled = true;
	}

	float LFO::Sample(unsigned sampleCount)
	{
		if (false == m_enabled)
			return 1.f;

		const unsigned sample = sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch;

		float signal = 0.f;
		switch (m_form)
		{
		default:
			SFM_ASSERT(false); // Unsupported oscillator

		case kCosine:
			signal = oscCos(phase);
			break;

		case kDigiPulse:
			signal = oscDigiPulse(phase, 0.25f); // Should be a parameter (FIXME)
			break;

		case kDigiSaw:
			signal = oscDigiSaw(phase);
			break;

		case kDigiTriangle:
			signal = oscDigiTriangle(phase);
			break;

		case kDigiSquare:
			signal = oscDigiSquare(phase);
			break;
		}

		SampleAssert(signal);

		return m_depth*signal;
	}
}
