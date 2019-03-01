
/*
	Syntherklaas FM - Voice.
*/

#include "synth-global.h"
#include "synth-voice.h"

namespace SFM
{
	void Voice::ResetOperators()
	{
		// NULL operators
		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			m_operators[iOp].Reset();
			m_feedbackBuf[iOp] = 0.f;
		}
	}

	// Full reset
	void Voice::Reset()
	{
		ResetOperators();

		// Disable
		m_state = kIdle;

		// LFO
		m_LFO = Oscillator();

		// Pitch env.
		m_pitchEnv.Reset();
		m_pitchEnvInvert = false;
		m_pitchEnvBias = 0.f;

		// Reset filter
		m_LPF.resetState();

		// Disable Wurlitzer mode
		m_wurlyMode = false;
	}

	void Voice::Release(float velocity)
	{
		for (auto &voiceOp : m_operators)
		{
			if (true == voiceOp.enabled)
				voiceOp.envelope.Stop(velocity);
		}

		m_state = kReleasing;
	}

	bool Voice::IsDone() /* const */
	{
		// Only true if all carrier envelopes are idle
		for (auto &voiceOp : m_operators)
		{
			if (true == voiceOp.enabled && true == voiceOp.isCarrier)
			{
				if (false == voiceOp.envelope.IsIdle())
					return false;
			}
		}

		return true;
	}

	float Voice::SummedOutput() /* const */
	{
		float summed = 0.f;
		for (auto &voiceOp : m_operators)
		{
			if (true == voiceOp.enabled && true == voiceOp.isCarrier)
			{
				summed += voiceOp.envelope.Get();
			}
		}

		return summed;
	}

	// Operator ovedrive distortion (sigmoid)
	SFM_INLINE float fOverdrive(float sample, float amount)
	{
		SFM_ASSERT(amount >= 0.f && amount <= 1.f);
		amount = 1.f + amount*15.f;
		const float distorted = atanf(sample*amount)*(2.f/kPI); // FIXME: LUT
		return distorted;
	}

	float Voice::Sample(const Parameters &parameters)
	{
		SFM_ASSERT(kIdle != m_state);

		// Sample LFO
		const float LFO = m_LFO.Sample(0.f);

		// Sample pitch envelope, and...
		float pitchEnv = m_pitchEnv.Sample();
		{
			// - Invert if necessary
			if (true == m_pitchEnvInvert) 
				pitchEnv = 1.f-pitchEnv;

			// - Bias it
			SFM_ASSERT(m_pitchEnvBias >= 0.f && m_pitchEnvBias <= 1.f);
			pitchEnv -= m_pitchEnvBias;

			// - Range 2 octaves [-1..+1]
			pitchEnv = powf(2.f, pitchEnv);

			// All of this: FIXME
		}

		// Process all operators top-down
		// This is a simple readable loop for R&D purposes, needs to be optimized later on (FIXME)

		float opSample[kNumOperators]; // Fully processed samples
		float mix = 0.f;               // Carrier mix
		float linAmp = 0.f;            // Linear amplitude

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
						modulation += opSample[iModulator];
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

					feedback = m_feedbackBuf[iFeedback]*voiceOp.feedbackAmt;
				}

				// Sample envelope
				const float envelope = voiceOp.envelope.Sample();

				// Apply LFO vibrato to envelope pitch
				float vibrato = parameters.pitchBend*pitchEnv;
				vibrato *= powf(2.f, LFO*parameters.modulation*voiceOp.pitchMod);
				
				// Bend oscillator pitch accordingly
				voiceOp.oscillator.PitchBend(vibrato);

				// Calculate sample
				float sample = voiceOp.oscillator.Sample(modulation+feedback);

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

				// Store final sample for modulation
				opSample[index] = sample;

				// If carrier: mix
				if (true == voiceOp.isCarrier)
				{
					linAmp += envelope;
					mix    += sample;

					++numCarriers;
				}
			}
		} 

		// Process feedback (not checking if operator is enabled, costs more)
		for (unsigned iOp = 0; iOp < kNumOperators; ++iOp)
		{
			// Divide sum by 4 and throw away a little bit (signal must slowly fade)
			m_feedbackBuf[iOp] = (kLeakyFactor*0.25f)*(m_feedbackBuf[iOp]+opSample[iOp]); 
		}

		// Normalize
		SFM_ASSERT(0 != numCarriers);
		const float invC = 1.f/numCarriers;
		mix    *= invC;
		linAmp *= invC;

		float filterAmt = 1.f;

		if (true == m_wurlyMode)
		{
			// Apply cheap distortion
			const float mixAsym = mix+0.3f;
			const float shaper = 1.f/(1.f + mixAsym*mixAsym*mixAsym);
			mix *= shaper;

			// Shape amplitude and use it as filter amount (maybe a bit hairy from user POV)
			const float powAmp = powf(linAmp, 3.f);
			filterAmt = powAmp;
		}

		// Apply filter
		float filtered = float(m_LPF.tick(mix));
		
		// FIXME: The filter has a problem in that it'll go every so slightly out of bounds
		//        Either that or it's the conversion to single precision
//		filtered = Clamp(filtered);

		mix = lerpf<float>(mix, filtered, filterAmt);

//		SampleAssert(mix);

		return mix;
	}
}
