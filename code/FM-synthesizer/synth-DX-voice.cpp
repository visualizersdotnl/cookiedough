
/*
	Syntherklaas FM - DX-style voice.
*/

#include "synth-global.h"
#include "synth-DX-voice.h"

namespace SFM
{
	float DX_Voice::Sample(const Parameters &parameters)
	{
		SFM_ASSERT(true == m_enabled);

		// Modulation vibrato & ADSR
		const float modVibrato = m_modVibrato.Sample(0.f);
		const float modADSR = 1.f; // m_modADSR.Sample(); (FIXME)

		// Step 1: process all operators top-down
		float sampled[kNumOperators];

		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			// Top-down
			const unsigned index = (kNumOperators-1)-iOp;

			Operator &opDX = m_operators[index];

			if (true == opDX.enabled)
			{
				// Get modulation
				float modulation = 0.f;
				if (-1 != opDX.routing)
				{
					const unsigned iModulator = opDX.routing;

					// Sanity checks
					SFM_ASSERT(iModulator < kNumOperators);
					SFM_ASSERT(iModulator > index);
					SFM_ASSERT(true == m_operators[index].enabled);

					// Get sample
					modulation = sampled[iModulator];

					// Apply ADSR & vibrato
					modulation *= modVibrato;
					modulation *= modADSR;
				}

				// Calculate sample
				float sample;
				if (false == opDX.isSlave)
					// Straight up
					sample = opDX.oscillator.Sample(modulation, m_pulseWidth);
				else
					// Variable modulation & lowpass
					sample = opDX.filter.Apply(opDX.oscillator.Sample(opDX.modAmount*modulation, m_pulseWidth));
	
				sampled[index] = sample;
			}
			else
			{
				// FIXME: defensive programming; can be deleted
				sampled[index] = 0.f;
			}
		}

		// Step 2: mix carriers
		float mix = 0.f;
	 	for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			// Bottom-up
			const unsigned index = iOp;

			Operator &opDX = m_operators[index];

			if (true == opDX.enabled && true == opDX.isCarrier)
			{
				// Mute all wavetable carriers if designated as one-shot (FIXME: it is a bit harder to prevent this but it can be done more efficiently than right here in this loop)
				float muteMul = 1.f;
				if (true == m_oneShot && true == oscIsWavetable(opDX.oscillator.GetWaveform()) && true == opDX.oscillator.HasCycled())
					muteMul = 0.f;

				mix = SoftClamp(mix + sampled[index]*muteMul);
			}
		}

		float sample = mix;

		// Add noise
		sample = SoftClamp(sample + parameters.m_noisyness*oscWhiteNoise());

		// Finally, modulate amplitude ('tremolo')
		sample *= m_AM.Sample(0.f, m_pulseWidth);

		SampleAssert(sample);

		return sample;
	}
}
