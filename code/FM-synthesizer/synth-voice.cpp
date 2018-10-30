
/*
	Syntherklaas FM - Voice.
*/

#include "synth-global.h"
#include "synth-voice.h"

namespace SFM
{
	SFM_INLINE void CopyCarrier(const Carrier &from, Carrier &to, float freqMul)
	{
		to.Initialize(from.m_sampleOffs, from.m_form, from.m_amplitude, from.m_frequency*freqMul);
	}

	SFM_INLINE void CopyModulator(const Modulator &from, Modulator &to, float freqMul)
	{
		to.Initialize(from.m_sampleOffs, from.m_index, from.m_frequency*freqMul, from.m_phaseShift, from.m_indexLFO.GetFrequency());
	}

	void Voice::InitializeFeedback()
	{
		// Up/down 4 halftones
		const float up = powf(2.f, 4.f/12.f);
		const float down = 1.f/up;
		CopyCarrier(m_carrierA, m_carrierHi, up);
		CopyCarrier(m_carrierA, m_carrierLo, down);
		CopyModulator(m_modulator, m_modulatorHi, up);
		CopyModulator(m_modulator, m_modulatorLo, down);
	}

	float Voice::Sample(unsigned sampleCount, float pitchBend, float modBrightness, ADSR &envelope, float pulseWidth)
	{
		// Assumption: any artifacts this yields are migitated by soft clamping down the line
//		SFM_ASSERT(pitchBend >= -1.f && pitchBend <= 1.f);

		// Silence one-shots (FIXME: is this necessary?)
		if (true == m_oneShot && true == HasCycled(sampleCount))
			return 0.f;

		const float ADSR = envelope.SampleForVoice(sampleCount);
		
		const float modulation = m_modulator.Sample(sampleCount, modBrightness);

		// Sample carrier(s)
		float sample = 0.f;
		switch (m_algorithm)
		{
		case kSingle:
			sample = m_carrierA.Sample(sampleCount, modulation, pulseWidth);
			break;

		case kDoubleCarriers:
			sample = fast_tanhf(m_carrierA.Sample(sampleCount, modulation, pulseWidth) + m_carrierB.Sample(sampleCount, -modulation, pulseWidth));
			break;
		}

		// Apply pitch bend
		if (pitchBend < 0.f)
		{
			const float delta = pitchBend+1.f;
			const float modulation = m_modulatorLo.Sample(sampleCount, modBrightness);
			const float low = m_carrierLo.Sample(sampleCount, modulation);
			sample = lerpf<float>(low, sample, delta);
		}
		else if (pitchBend > 0.f)
		{
			const float delta = pitchBend;
			const float modulation = m_modulatorHi.Sample(sampleCount, modBrightness);
			const float high = m_carrierHi.Sample(sampleCount, modulation);
			sample = lerpf<float>(sample, high, pitchBend);
		}
		
		sample *= ADSR;

		// Finally, modulate amplitude ('tremolo')
		sample *= m_ampMod.Sample(sampleCount, 0.f);

		SFM_ASSERT(sample >= -1.f && sample <= 1.f);
		SFM_ASSERT(true == FloatCheck(sample));

		return sample;
	}
}