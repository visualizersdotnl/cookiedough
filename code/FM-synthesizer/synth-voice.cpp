
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
		to.Initialize(from.m_sampleOffs, from.m_index, from.m_frequency*freqMul, from.m_phaseShift, from.m_indexModFreq);
	}

	void Voice::InitializeFeedback()
	{
		// Up/down 3 halftones
		const float up = powf(2.f, 3.f/12.f);
		const float down = 1.f/up;
		CopyCarrier(m_carrier, m_carrierHi, up);
		CopyCarrier(m_carrier, m_carrierLo, down);
		CopyModulator(m_modulator, m_modulatorHi, up);
		CopyModulator(m_modulator, m_modulatorLo, down);
	}

	float Voice::Sample(unsigned sampleCount, float pitchBend, float modBrightness, ADSR &envelope)
	{
		// Assumption: any artifacts this yields are migitated by soft clamping down the line
//		SFM_ASSERT(pitchBend >= -1.f && pitchBend <= 1.f);

		const float ADSR = envelope.SampleForVoice(sampleCount);
		
		float modulation = m_modulator.Sample(sampleCount, modBrightness);
		float sample = m_carrier.Sample(sampleCount, modulation);

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

		return sample;
	}
}