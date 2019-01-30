
/*
	Syntherklaas FM - Voice.
*/

#include "synth-global.h"
#include "synth-voice.h"

namespace SFM
{
	// FIXME: these numbers are of no particular significance (yet)
	const float kDefPickupDist = 0.314f*2.f; // 1.1f;
	const float kDefPickupAsym = 0.314f; // 0.3f;

	// Pickup distortion
	SFM_INLINE float fPickup(float signal, float distance, float asymmetry)
	{
		SampleAssert(signal);
		const float z = signal+asymmetry;
		return 1.f/(distance + z*z*z);
	}
	// Ovedrive distortion
	SFM_INLINE float fOverdrive(float sample, float amount)
	{
		SFM_ASSERT(amount >= 0.f && amount <= 1.f);
		amount = 1.f + amount*31.f;
		const float distorted = atanf(sample*amount)*(2.f/kPI);
		return distorted;
	}

	float Voice::Sample(const Parameters &parameters)
	{
		SFM_ASSERT(kIdle != m_state);

		// Sample LFO
		const float LFO = m_LFO.Sample(0.f);

		// Process all operators top-down
		// This is a simple, readable loop which needs to be optimized later on (FIXME)

		float sampled[kNumOperators];

		float mix = 0.f;
		float linAmp = 0.f;
		unsigned numCarriers = 0;
		for (int iOp = kNumOperators-1; iOp >= 0; --iOp)
		{
			// Top-down
			const unsigned index = iOp;

			Operator &voiceOp = m_operators[index];

			if (true == voiceOp.enabled)
			{
				// Get modulation (from 3 sources maximum; FIXME: too much?)
				float modulation = 0.f;
				for (unsigned iMod = 0; iMod < 3; ++iMod)
				{
					const unsigned iModulator = voiceOp.modulators[iMod];
					if (-1 != iModulator)
					{
						// Sanity checks
						SFM_ASSERT(iModulator < kNumOperators);
						SFM_ASSERT(iModulator > index);
						SFM_ASSERT(true == m_operators[iModulator].enabled);

						// Get sample
						modulation += sampled[iModulator];
					}
				}

				// Get feedback
				float feedback = 0.f;
				if (voiceOp.feedback != -1)
				{
					const unsigned iFeedback = voiceOp.feedback;

					// Sanity checks
					SFM_ASSERT(iFeedback < kNumOperators);
					SFM_ASSERT(true == m_operators[iFeedback].enabled);

					feedback = m_feedback[index];
				}

				// Sample envelope
				const float envelope = voiceOp.envelope.Sample();
				SampleAssert(envelope);

				// Apply LFO vibrato
				modulation += LFO*parameters.modulation*voiceOp.pitchMod;

				// Calculate sample
				float sample = voiceOp.oscillator.Sample(modulation + feedback);

				// Apply LFO tremolo
				const float tremolo = lerpf<float>(1.f, LFO, voiceOp.ampMod);
				sample = lerpf<float>(sample, sample*tremolo, parameters.modulation);
				
				// Apply envelope
				sample = sample*envelope;

				// Apply distortion without losing amplitude
				const float distAmt = voiceOp.distortion;
				const float overdrive = fOverdrive(sample, distAmt);
				sample = lerpf<float>(sample, overdrive, distAmt);

				SampleAssert(sample);

				// Store final sample for modulation and feedback
				sampled[index] = sample;

				// If carrier: apply tremolo & mix
				if (true == voiceOp.isCarrier)
				{
					linAmp += envelope;
					mix += sample;
					++numCarriers;
				}
			}
		} 

		float filterAmt;
		switch (m_mode)
		{
		case kFM:
			// Scale voice by number of carriers
			SFM_ASSERT(0 != numCarriers);
			mix /= numCarriers;
			filterAmt = 1.f;
			break;

		case kPickup:
			{
				// Check if algorithm adheres to mode constraints
				// For now that means operator #1 is a wave shaper *only*
				SFM_ASSERT(1 == numCarriers);
				SFM_ASSERT(true == m_operators[0].isCarrier);
//				SFM_ASSERT(0.f == m_operators[0].oscillator.GetFrequency());

				const float pickup = fPickup(mix, kDefPickupDist, kDefPickupAsym);
				mix = lerpf<float>(mix, mix*pickup, parameters.pickupAmt);
				filterAmt = linAmp;
			}

			break;
		}

		// Store feedback (https://www.reddit.com/r/FMsynthesis/comments/85jfrb/dx7_feedback_implementation/)
	 	for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			const float sample = sampled[iOp] * m_operators[iOp].feedbackAmt;
			const float previous = m_feedback[iOp];
			const float feedback = 0.25f*(previous+sample); // This limits it all nicely
			m_feedback[iOp] = feedback;
		}

		// Apply filter
		const float filtered = float(m_LPF.tick(mix));
		mix = lerpf<float>(mix, filtered, filterAmt);

		SampleAssert(mix);

		return mix;
	}
}
