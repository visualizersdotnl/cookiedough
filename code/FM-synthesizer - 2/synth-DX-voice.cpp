
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

		// Get global ADSR
		float ADSR = m_ADSR.Sample();

		// Get tremolo
		const float tremolo = m_tremolo.Sample(0.f);

		// Get vibrato
		const float vibrato = powf(2.f, m_vibrato.Sample(0.f)*ADSR);

		// Process all operators top-down (this isn't too pretty but good enough for our amount of operators)
		float sampled[kNumOperators];

		float mix = 0.f;
		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			// Top-down
			const unsigned index = (kNumOperators-1)-iOp;

			Operator &opDX = m_operators[index];

			if (true == opDX.enabled)
			{
				// Get modulation (from 3 sources maximum, FIXME: get rid of this if it ain't used)
				float modulation = 0.f;
				for (unsigned iMod = 0; iMod < 3; ++iMod)
				{
					if (-1 != opDX.modulators[iMod])
					{
						const unsigned iModulator = opDX.modulators[iMod];

						// Sanity checks
						SFM_ASSERT(iModulator < kNumOperators);
						SFM_ASSERT(iModulator > index);
						SFM_ASSERT(true == m_operators[iModulator].enabled);

						// Get sample
						modulation += sampled[iModulator];
					}
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

				// Set pitch bend
				float bend = m_pitchBend + opDX.vibrato*vibrato;
				opDX.oscillator.PitchBend(bend);

				// Calculate sample (FIXME: is this the right spot?)
				float sample = opDX.oscillator.Sample(modulation);

				// Store sample for feedback at this point; feels like a sane spot: straight out of the oscillator
				sampled[index] = sample;

				// Factor in tremolo
				sample = lerpf<float>(sample, sample*tremolo, opDX.tremolo);

				// And the mod. envelope
				sample *= opDX.opEnv.Sample();

				// If carrier: mix
				if (true == opDX.isCarrier)
					mix = SoftClamp(mix + sample);
			}
		}

		// Integrate feedback
		// The DX7 does this variably (by bit shift it seems checking Dexed), this works for us now!
	 	for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
			m_feedback[iOp] = m_feedback[iOp]*0.95f + sampled[iOp]*0.05f;

		// Factor in ADSR
		mix *= ADSR;

		SampleAssert(mix);


		return mix;
	}
}
