
/*
	Syntherklaas FM - Voice.
*/

#include "synth-global.h"
#include "synth-voice.h"

namespace SFM
{
	float Voice::Sample(unsigned sampleCount, float brightness, float noise, const float carrierVolumes[], float slaveFM)
	{
		// Silence one-shots
		const int firstCarrierMul = !(true == m_oneShot && true == HasCycled(sampleCount));

		// Get FM
		const float modulation = m_modulator.Sample(sampleCount, brightness);

		// Sample carrier(s)
		float sample = 0.f;
		const float pulseWidth = m_pulseWidth;
	
		switch (m_algorithm)
		{
		case kSingle:
			sample = m_carriers[0].Sample(sampleCount, modulation, pulseWidth) * firstCarrierMul;
			break;

		case kDoubleCarriers:
			sample  = fast_tanhf(m_carriers[0].Sample(sampleCount, modulation, pulseWidth) + carrierVolumes[0]*m_carriers[1].Sample(sampleCount, modulation, pulseWidth));
			sample *= firstCarrierMul; // Carrier #2 is a copy of #1
			break;

		case kMiniMOOG:
			{
				const float slaveMod = modulation*slaveFM;
				const float A = m_carriers[0].Sample(sampleCount, modulation, pulseWidth) * firstCarrierMul;
				const float B = m_carriers[1].Sample(sampleCount, slaveMod, pulseWidth);
				const float C = m_carriers[2].Sample(sampleCount, slaveMod, pulseWidth);
				sample = fast_tanhf(carrierVolumes[0]*A + carrierVolumes[1]*B + carrierVolumes[2]*C);
			}
			break;
		}

		// Add noise (can create nice distortion effects)
		const float pinkNoise = oscPinkNoise();
		sample = lerpf<float>(sample, sample*pinkNoise, noise);

		// Finally, modulate amplitude ('tremolo')
		sample *= m_ampMod.Sample(sampleCount, 0.f);

		SampleAssert(sample);

		return sample;
	}
}
