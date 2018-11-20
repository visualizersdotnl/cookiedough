
/*
	Syntherklaas FM - Yamaha DX style voice.
*/

#include "synth-global.h"
#include "synth-DX-voice.h"

namespace SFM
{
	float DX_Voice::Sample(const Parameters &parameters)
	{
		SFM_ASSERT(true == m_enabled);

		// Get vibrato
		const float vibrato = m_vibrato.Sample(0.f);

		// Get modulation env.
		const float modEnv = m_modADSR.Sample();

		// Process all operators top-down
		float sampled[kNumOperators];

		float mix = 0.f;
		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			// Top-down
			const unsigned index = (kNumOperators-1)-iOp;

			Operator &opDX = m_operators[index];

			if (true == opDX.enabled)
			{
				// Get modulation
				float modulation = 0.f;
				if (-1 != opDX.modulator)
				{
					const unsigned iModulator = opDX.modulator;

					// Sanity checks
					SFM_ASSERT(iModulator < kNumOperators);
					SFM_ASSERT(iModulator > index);
					SFM_ASSERT(true == m_operators[iModulator].enabled);

					// Get sample
					modulation = sampled[iModulator];
				}

				// Get feedback
				if (opDX.feedback != -1)
				{
					const unsigned iFeedback = opDX.feedback;

					// Sanity checks
					SFM_ASSERT(iFeedback < kNumOperators);
					SFM_ASSERT(true == m_operators[iFeedback].enabled);

					modulation += m_feedback[index];
				}

				// Factor in vibrato
				modulation = lerpf<float>(modulation, modulation*vibrato, opDX.vibrato);

				// And the envelope
				modulation *= modEnv;

				// Calculate sample
				float sample = opDX.oscillator.Sample(modulation);

				// If carrier: mix
				if (true == opDX.isCarrier)
					mix = SoftClamp(mix + sample);

				sampled[index] = sample;
			}
		}

		// Integrate feedback
	 	for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			m_feedback[iOp] =  m_feedback[iOp]*0.95f + sampled[iOp]*0.05f;

		SampleAssert(mix);

		// Global ADSR
		mix *= m_ADSR.Sample();

		return mix;
	}
}
