
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

		// No drift
		const float drift = 0.f;

		const float pulseWidth = m_pulseWidth;
	
		switch (m_algorithm)
		{
		case kSingle:
			sample = m_carriers[0].Sample(sampleCount, drift, modulation, pulseWidth) * firstCarrierMul;
			break;

		case kDoubleCarriers:
			sample  = fast_tanhf(m_carriers[0].Sample(sampleCount, drift, modulation, pulseWidth) + parameters.m_doubleVolume*m_carriers[1].Sample(sampleCount, drift, modulation, pulseWidth));
			sample *= firstCarrierMul; // Carrier #2 is a copy of #1
			break;

		case kMiniMOOG:
			{
				const float slaveMod = modulation*parameters.m_slaveFM; 
				const float A = m_carriers[0].Sample(sampleCount, drift, modulation, pulseWidth) * firstCarrierMul;
				const float B = m_carriers[1].Sample(sampleCount, drift, slaveMod, pulseWidth);
				const float C = m_carriers[2].Sample(sampleCount, drift, slaveMod, pulseWidth);

				const float *pVols = parameters.m_carrierVol;
 
				// Let us hope Sylvana Simons never reads this
				const float slaves = SoftClamp(pVols[1]*B + pVols[2]*C);
				const float filtered = m_LPF.Apply(slaves);

				sample = SoftClamp(pVols[0]*A + filtered);
			}
			break;
		}

		// Add noise
		sample = Clamp(sample + parameters.m_noisyness*oscWhiteNoise(modulation));

		// Finally, modulate amplitude ('tremolo')
		sample *= m_AM.Sample(sampleCount, 0.f, 0.f, pulseWidth);

		SampleAssert(sample);

		return sample;
	}
}
