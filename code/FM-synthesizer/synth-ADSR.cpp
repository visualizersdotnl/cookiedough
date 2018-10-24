
/*
	Syntherklaas FM -- ADSR envelope.
*/

#include "synth-global.h"
#include "synth-ADSR.h"

namespace SFM
{
	const unsigned kMinReleaseSamples = 128;

	void ADSR::Start(unsigned sampleCount, const Parameters &parameters, float velocity)
	{
		m_sampleOffs = sampleCount;
		m_sustain = parameters.sustain;

		// Scale envelope by note velocity (with "bro science")
		SFM_ASSERT(velocity <= 1.f);
		const float velScale = 0.314f + (1.f-0.314f)*velocity;
		m_attack  = unsigned(parameters.attack*kSampleRate*velScale);
		m_decay   = unsigned(parameters.decay*kSampleRate);
		m_release = unsigned(parameters.release*kSampleRate*velScale);
		
		// Always at least for an amount of samples to avoid pop/click when 0
		m_release = std::max<unsigned>(kMinReleaseSamples, m_release);

		m_curAmp = 0.f;
		m_isReleasing = false; // Not in release stage
	}

	void ADSR::Stop(unsigned sampleCount)
	{
		// Always use current amplitude for release
		m_sustain = m_curAmp;

		m_sampleOffs = sampleCount;
		m_isReleasing = true;
	}

	float ADSR::Sample(unsigned sampleCount)
	{
		unsigned sample = sampleCount-m_sampleOffs;

		if (false == m_isReleasing)
		{
			if (sample < m_attack)
			{
				// Build up to full attack (linear)
				const float delta = 1.f/m_attack;
				const float step = delta*sample;
				m_curAmp = step;
				if (m_curAmp > 1.f) m_curAmp = 1.f;
			}
			else if (sample >= m_attack && sample < m_attack+m_decay)
			{
				// Decay to sustain (linear)
				sample -= m_attack;
				const float delta = 1.f/m_decay;
				const float step = delta*sample;
				m_curAmp = lerpf<float>(1.f, m_sustain, step);
			}
			else
				return m_curAmp;
		}
		else
		{
			// Sustain level and sample offset are adjusted on NOTE_OFF (linear)
			if (sample < m_release)
			{
				const float delta = 1.f/m_release;
				const float step = delta*sample;
				m_curAmp = lerpf<float>(m_sustain, 0.f, step);
				if (m_curAmp < 0.f) m_curAmp = 0.f;
			}
			else
				m_curAmp = 0.f;
		}

		return m_curAmp;
	}
}