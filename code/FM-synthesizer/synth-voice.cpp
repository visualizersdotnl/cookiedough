
/*
	Syntherklaas FM - Voice.
*/

#include "synth-global.h"
#include "synth-voice.h"

namespace SFM
{
#if 0

	SFM_INLINE void CopyCarrier(const Carrier &from, Carrier &to, float freqMul)
	{
		to.Initialize(from.m_sampleOffs, from.m_form, from.m_amplitude, from.m_frequency*freqMul);
	}

	SFM_INLINE void CopyModulator(const Modulator &from, Modulator &to, float freqMul)
	{
		to.Initialize(from.m_sampleOffs, from.m_index, from.m_frequency*freqMul, from.m_phaseShift, from.m_indexLFO.GetFrequency());
	}

	void Voice::InitializePitchBend()
	{
		// Up/down 1 semitone
		const float up = powf(2.f, 1.f/12.f);
		const float down = 1.f/up;
		CopyCarrier(m_carrierA, m_carrierHi, up);
		CopyCarrier(m_carrierA, m_carrierLo, down);
		CopyModulator(m_modulator, m_modulatorHi, up);
		CopyModulator(m_modulator, m_modulatorLo, down);
	}

#endif

	float Voice::Sample(unsigned sampleCount, float brightness, ADSR &envelope, float noise, const float carrierVolumes[])
	{
		// Assumption: any artifacts this yields are migitated by soft clamping down the line
//		SFM_ASSERT(pitchBend >= -1.f && pitchBend <= 1.f);

		// Silence one-shots
		const int firstCarrierMul = !(true == m_oneShot && true == HasCycled(sampleCount));

		const float ADSR = envelope.Sample(sampleCount);
		
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
			sample = fast_tanhf(m_carriers[0].Sample(sampleCount, modulation, pulseWidth) + carrierVolumes[0]*m_carriers[1].Sample(sampleCount, modulation, pulseWidth));
			break;

		case kMiniMOOG:
			{
				const float A = m_carriers[0].Sample(sampleCount, modulation, pulseWidth) * firstCarrierMul;
				const float B = m_carriers[1].Sample(sampleCount, modulation, pulseWidth);
				const float C = m_carriers[2].Sample(sampleCount, modulation, pulseWidth);
				sample = fast_tanhf(carrierVolumes[0]*A + carrierVolumes[1]*B + carrierVolumes[2]*C);
			}
			break;
		}

		// Add noise (can create nice distortion effects)
		sample += sample*noise*oscBrownishNoise();
		
		// Factor in ADSR
		sample *= ADSR;

		// Finally, modulate amplitude ('tremolo')
		sample *= m_ampMod.Sample(sampleCount, 0.f);

		// This ain't that alarming since we clamp just about everywhere
//		SFM_ASSERT(sample >= -1.f && sample <= 1.f);
	
		SFM_ASSERT(true == FloatCheck(sample));

		return sample;
	}
}
