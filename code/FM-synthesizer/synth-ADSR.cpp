
/*
	Syntherklaas FM -- ADSR envelope.
*/

#include "synth-global.h"
#include "synth-ADSR.h"

namespace SFM
{
	void ADSR::Start(unsigned sampleCount, const Parameters &parameters, float velocity)
	{
		m_ADSR.reset();

		m_sampleOffs = sampleCount;

		// 25% longer attack & 50% longer release on max. velocity
		const float attackScale  = 1.f + 0.25f*velocity;
		const float releaseScale = 1.f + 0.5f*velocity;
		
		const float attack  = 2.f + truncf(attackScale*parameters.attack*kSampleRate);
		const float decay   = 2.f + truncf(parameters.decay*kSampleRate);
		const float release = std::max<float>(kSampleRate/500.f /* 2ms. min */, truncf(releaseScale*parameters.release*kSampleRate)); // Velocity decides not only volume but also release
		const float sustain = parameters.sustain;

		m_ADSR.setAttackRate(attack);
		m_ADSR.setDecayRate(decay);
		m_ADSR.setReleaseRate(release);
		m_ADSR.setSustainLevel(sustain);

		m_ADSR.gate(true);
	}

	void ADSR::Stop(unsigned sampleCount, float velocity)
	{
		// Go into release state.
		m_ADSR.gate(false);
	}

	/*
		ADSR_Simple
	*/

	void ADSR_Simple::Start(unsigned sampleCount, const Parameters &parameters, float velocity)
	{
		Reset();

		m_sampleOffs = sampleCount;

		m_attack  = unsigned(parameters.attack*kSampleRate);
		m_decay   = unsigned(parameters.decay*kSampleRate);
		m_release = std::max<unsigned>(kSampleRate/1000 /* 1ms */, unsigned(parameters.release*kSampleRate));
		m_sustain = parameters.sustain;

		m_state = kAttack;
		m_output = 0.f;
	}

	void ADSR_Simple::Stop(unsigned sampleCount)
	{
		m_sampleOffs = sampleCount;
		m_state = kRelease;

		// Should be equal, but I'll be defensive
		m_sustain = m_output;
	}

	void ADSR_Simple::Reset()
	{
		m_state = kIdle;
		m_output = 0.f;
	}

	float ADSR_Simple::SampleForVoice(unsigned sampleCount)
	{
		const unsigned sample = sampleCount-m_sampleOffs;

		switch (m_state)
		{
		case kAttack: // Linear
			{
				const float step = 1.f/m_attack;
				const float delta = sample*step;
				m_output = std::min<float>(1.f, delta);
				if (sample >= m_attack)
				{
					m_state = kDecay;
				}

				break;
			}

		case kDecay: // Exponential
			{
				const float step = 1.f/m_decay;
				const float delta = (sample-m_attack)*step;
				m_output = std::max<float>(m_sustain, lerpf(1.f, m_sustain, delta*delta));

				if (sample >= m_attack+m_decay)
				{
					m_state = kSustain;
				}

				break;
			}

		case kSustain:
			{
				break;
			}

		case kRelease: // Smoothstep
			{
				const float step = 1.f/m_release;
				const float delta = sample*step;
				m_output = lerpf(m_sustain, 0.f, smoothstepf(delta));
				if (m_output <= 0.f)
				{
					m_output = 0.f;
					m_state = kIdle;
				}

				break;
			}

		case kIdle:
			m_output = 0.f;
			break;
		}

		SFM_ASSERT(m_output >= 0.f && m_output <= 1.f);
		FloatCheck(m_output);

		return m_output;
	}	

	float SampleForFilter(unsigned sampleCount) { return 1.f; } // FIXME
}
