
/*
	Syntherklaas FM -- ADSR envelope.
*/

#include "synth-global.h"
#include "synth-ADSR.h"

namespace SFM
{
	void ADSR::Start(unsigned sampleCount, const Parameters &parameters, float velocity)
	{
		SFM_ASSERT(velocity <= 1.f);

		m_sampleOffs = sampleCount;

		// Not in release stage
		m_releasing = false;

		// Scale envelope by note velocity (with "bro science")
		SFM_ASSERT(velocity != 0.f);
		const float velScale = 0.314f + (1.f-0.314f)*velocity;
		m_parameters.attack  = unsigned(parameters.attack*velScale);
		m_parameters.decay   = unsigned(parameters.decay*velScale);
		
		// Always at least for an amount of samples to avoid pop/click when 0
		m_parameters.release = std::max<unsigned>(kSampleRate/16, unsigned(parameters.release));
		
		m_parameters.sustain = parameters.sustain;
	}

	void ADSR::Stop(unsigned sampleCount)
	{
		// Always use current amplitude for release
		m_parameters.sustain = m_curAmp;

		m_sampleOffs = sampleCount;
		m_releasing = true;
	}

	float ADSR::Sample(unsigned sampleCount)
	{
		const unsigned sample = sampleCount-m_sampleOffs;

		const unsigned attack  = m_parameters.attack;
		const unsigned decay   = m_parameters.decay;
		const unsigned release = m_parameters.release;

		const float sustain = m_parameters.sustain;

		float amplitude = 0.f;

		if (false == m_releasing)
		{
			if (sample < attack)
			{
				// Build up to full attack (linear)
				const float step = 1.f/attack;
				const float delta = step*sample;
				amplitude = delta;
				SFM_ASSERT(amplitude >= 0.f && amplitude <= 1.f);
			}
			else if (sample >= attack && sample < attack+decay)
			{
				// Decay to sustain (exponential)
				const float step = 1.f/decay;
				const float delta = step*(sample-attack);
				amplitude = 1.f - sustain*(delta*delta);
				SFM_ASSERT(amplitude <= 1.f /* && amplitude >= sustain */);
			}
			else
				return m_curAmp;
		}
		else
		{
			// Sustain level and sample offset are adjusted on NOTE_OFF (exponential)
			if (sample < release)
			{
				const float step = sustain/release;
				const float delta = step*sample;
				amplitude = sustain - sustain*(delta*delta);
				SFM_ASSERT(amplitude >= 0.f && amplitude <= sustain);
			}
		}

		SFM_ASSERT(amplitude >= 0.f && amplitude <= 1.f);
		m_curAmp = amplitude;

		return amplitude;
	}
}
