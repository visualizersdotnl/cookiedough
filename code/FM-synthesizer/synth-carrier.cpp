
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
		m_form = form;
		m_amplitude = amplitude;
		m_pitch = CalculateOscPitch(frequency);
		m_angularPitch = CalculateAngularPitch(frequency);
		m_sampleOffs = sampleCount;
		m_numHarmonics = GetCarrierHarmonics(frequency);
	}

	float Carrier::Sample(unsigned sampleCount, float modulation)
	{
		const unsigned sample = sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch;

		// Convert modulation to LUT period
		modulation *= kRadToOscLUT;

		// FIXME: get rid of switch!
		float signal = 0.f;
		switch (m_form)
		{
		case kSine:
			signal = oscSine(phase+modulation);
			break;

		case kSoftSaw:
			signal = oscSoftSaw(phase+modulation, m_numHarmonics);
			break;

		case kSoftSquare:
			signal = oscSoftSquare(phase+modulation, m_numHarmonics);
			break;

		case kDigiSaw:
			signal = oscDigiSaw(phase+modulation);
			break;

		case kDigiSquare:
			signal = oscDigiSquare(phase+modulation);
			break;

		case kTriangle:
			signal = oscTriangle(phase+modulation);
			break;

		case kWhiteNoise:
			signal = oscWhiteNoise(phase);
			break;

		case kPinkNoise:
			signal = oscPinkNoise(phase);
			break;
		}

		return m_amplitude*signal;
	}
}
