
/*
	Syntherklaas FM - Voice.
*/

#include "synth-global.h"
#include "synth-voice.h"

namespace SFM
{
	float Voice::Sample(unsigned sampleCount, const Parameters &parameters)
	{
		// Silence one-shots
		const int firstCarrierMul = !(true == m_oneShot && true == HasCycled(sampleCount));

		// Get FM
		const float modulation = m_modulator.Sample(sampleCount, parameters.m_modBrightness);

		// Sample carrier(s)
		float sample = 0.f;
		const float pulseWidth = m_pulseWidth;
	
		switch (m_algorithm)
		{
		case kSingle:
			sample = m_carriers[0].Sample(sampleCount, modulation, pulseWidth) * firstCarrierMul;
			break;

		case kDoubleCarriers:
			sample  = fast_tanhf(m_carriers[0].Sample(sampleCount, modulation, pulseWidth) + parameters.m_doubleVolume*m_carriers[1].Sample(sampleCount, modulation, pulseWidth));
			sample *= firstCarrierMul; // Carrier #2 is a copy of #1
			break;

		case kMiniMOOG:
			{
				const float slaveMod = modulation*parameters.m_slaveFM;
				const float A = m_carriers[0].Sample(sampleCount, modulation, pulseWidth) * firstCarrierMul;
				const float B = m_carriers[1].Sample(sampleCount, slaveMod, pulseWidth);
				const float C = m_carriers[2].Sample(sampleCount, slaveMod, pulseWidth);

				const float *pVols = parameters.m_carrierVol;
				sample = Mix(pVols[0]*A, Mix(pVols[1]*B, pVols[2]*C));
			}
			break;
		}

		// Add noise (can create nice distortion effects)
		sample = lerpf<float>(sample, sample*oscWhiteNoise(), parameters.m_noisyness);

		// Finally, modulate amplitude ('tremolo')
		sample *= m_ampMod.Sample(sampleCount);

		SampleAssert(sample);

		return sample;
	}
}
