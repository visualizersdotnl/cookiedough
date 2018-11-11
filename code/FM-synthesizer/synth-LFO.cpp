
/*
	Syntherklaas FM -- Simple continuous oscillator (LFO).
*/

#pragma once

#include "synth-global.h"
#include "synth-LFO.h"

namespace SFM
{
	void LFO::Initialize(unsigned sampleCount, Waveform form, float amplitude, float frequency)
	{
		SFM_ASSERT(frequency <= kMaxSonicLFO);

		m_sampleOffs = sampleCount;
		m_form = form;
		m_amplitude = amplitude;
		m_frequency = frequency;
		m_pitch = CalculatePitch(frequency);
	}

	float LFO::Sample(unsigned sampleCount)
	{
		const unsigned sample = sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch;

		float signal = 0.f;
		switch (m_form)
		{
		default:
			// Unsupported oscillator
			signal = 0.f;
			SFM_ASSERT(false); 
			break;

		case kSine:
			signal = oscSine(phase);
			break;

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

		case kWhiteNoise:
			signal = oscWhiteNoise(phase);
			break;
		}

		SampleAssert(signal);

		return m_amplitude*signal;
	}
}
