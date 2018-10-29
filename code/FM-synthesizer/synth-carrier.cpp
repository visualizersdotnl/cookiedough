
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
		m_frequency = frequency;
		m_pitch = CalculateOscPitch(frequency);
		m_sampleOffs = sampleCount;
		m_numHarmonics = GetCarrierHarmonics(frequency);

		m_cycleLen = -1.f;
	
		switch (m_form)
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
			m_cycleLen = kOscPeriod;
			break;
		}

		SFM_ASSERT(-1 != m_cycleLen);
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
		default:
		case kSine:
			signal = oscSine(phase+modulation);
			break;

		case kSoftSaw:
			signal = oscSoftSaw(phase+modulation, m_numHarmonics);
			break;

		case kSoftSquare:
			signal = oscSoftSquare(phase+modulation, m_numHarmonics);
			break;

/*
		case kDigiSaw:
			signal = oscDigiSaw(phase+modulation);
			break;

		case kDigiSquare:
			signal = oscDigiSquare(phase+modulation);
			break;
*/

		case kTriangle:
			signal = oscTriangle(phase+modulation);
			break;

/*
		case kWhiteNoise:
			signal = oscWhiteNoise(phase);
			break;

		case kPinkNoise:
			signal = oscPinkNoise(phase);
			break;
*/

		case kKick808:
			signal = getOscKick808().Sample(phase+modulation);
			break;

		case kSnare808:
			signal = getOscSnare808().Sample(phase+modulation);
			break;

		case kGuitar:
			signal = getOscGuitar().Sample(phase+modulation);
			break;

		case kElectricPiano:
			signal = getOscElecPiano().Sample(phase+modulation);
			break;
		}

		const float wave = m_amplitude*signal;
		SampleAssert(wave);

		return wave;
	}
}
