
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
		const float releaseScale = 1.f + 2.f*velocity;
		
		// Attack & release have a minimum to prevent clicking
		const float attack  = truncf(attackScale*parameters.attack*kSampleRate);
		const float decay   = truncf(parameters.decay*kSampleRate);
		const float release = std::max<float>(attack+decay > 0 ? kSampleRate/500.f /* 2ms min. */ : 0, truncf(releaseScale*parameters.release*kSampleRate));
		const float sustain = parameters.sustain;

		// Harder touch, less linear
		const float invVel = 1.f-velocity;
		const float goldenOffs = kGoldenRatio*0.1f;
		m_ADSR.setTargetRatioA(goldenOffs + invVel);
		m_ADSR.setTargetRatioDR(invVel);

		m_ADSR.setAttackRate(attack);
		m_ADSR.setDecayRate(decay);
		m_ADSR.setReleaseRate(release);
		m_ADSR.setSustainLevel(sustain);

		m_ADSR.gate(true);
	}

	void ADSR::Stop(unsigned sampleCount, float velocity)
	{
		// Harder touch, less linear
		const float invVel = 1.314f-velocity;
		m_ADSR.setTargetRatioDR(invVel);

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

		m_attack  = 16+unsigned(parameters.attack*kSampleRate);
		m_decay   = 16+unsigned(parameters.decay*kSampleRate);
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

	float ADSR_Simple::Sample(unsigned sampleCount)
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

		case kRelease: // Exponential
			{
				const float step = 1.f/m_release;
				const float delta = sample*step;
				m_output = lerpf(m_sustain, 0.f, delta*delta);
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
