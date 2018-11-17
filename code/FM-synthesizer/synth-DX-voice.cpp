
/*
	Syntherklaas FM - Yamaha DX style voice.

	Voice-specific to do:
		- What happens if I modulate the mod. vibrato LFO with the A(D)SR envelope?
		- Get rid of ugly mute condition (in second loop now)
		- To optimize, firstly, the 2 loops can be collapsed into one
		- This will all run faster when Oscillator is optimized
		- There is a lot of boolean logic that seems excessive
		- Add voice amplitude ADSR to class!
*/

#include "synth-global.h"
#include "synth-DX-voice.h"

namespace SFM
{
	float DX_Voice::Sample(const Parameters &parameters)
	{
		SFM_ASSERT(true == m_enabled);

		// Modulation vibrato & envelope
		const float modVibrato = m_modVibrato.Sample(0.f);
		const float modEnv = m_modADSR.Sample();

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
				if (-1 != opDX.modulator)
				{
					const unsigned iModulator = opDX.modulator;

					// Sanity checks
					SFM_ASSERT(iModulator < kNumOperators);
					SFM_ASSERT(iModulator > index);
					SFM_ASSERT(true == m_operators[iModulator].enabled);

					// Get sample
					modulation = sampled[iModulator];

					// Apply vibrato
					modulation = lerpf<float>(modulation, modulation*modVibrato, opDX.vibrato);
				}

				// Apply ADSR (FIXME: before or after feedback?)
				// As it stands the feedback has a low frequency so I do not think I should
				modulation *= modEnv;

				// Get feedback
				if (opDX.feedback != -1)
				{
					const unsigned iFeedback = opDX.feedback;

					// Sanity checks
					SFM_ASSERT(iFeedback < kNumOperators);
					SFM_ASSERT(true == m_operators[iFeedback].enabled);

					const float feedback = m_operators[iFeedback].prevSample;

					modulation += feedback;
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
		}

		// Step 2: mix carriers & store samples for feedback
		float mix = 0.f;
	 	for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			// Bottom-up
			const unsigned index = iOp;

			Operator &opDX = m_operators[index];

			if (true == opDX.enabled)
			{
				// Mute carriers that are done (one-shot)
				float muteMul = 1.f;
				if (true == opDX.oscillator.IsDone())
					muteMul = 0.f;

				const float sample = sampled[index]*muteMul;

				// Is carrier: mix
				if (true == opDX.isCarrier)
				{
					mix = SoftClamp(mix + sample);
				}

				// Leaky integrate (I always think this looks sloppy but it works)
				opDX.prevSample = opDX.prevSample*0.9f + sample*0.1f;
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
