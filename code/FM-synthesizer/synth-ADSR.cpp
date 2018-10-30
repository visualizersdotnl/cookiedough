
/*
	Syntherklaas FM -- ADSR envelope.
*/

#include "synth-global.h"
#include "synth-ADSR.h"

namespace SFM
{
	void ADSR::Start(unsigned sampleCount, const Parameters &parameters, float velocity)
	{
		m_voiceADSR.reset();
		m_filterADSR.reset();

		m_sampleOffs = sampleCount;

		const float attack  = truncf(parameters.attack*kSampleRate);
		const float decay   = truncf(parameters.decay*kSampleRate);
		const float release = std::max<float>(kSampleRate/500.f /* 2ms. min */, truncf(parameters.release*kSampleRate));
		const float sustain = parameters.sustain;

		// Set attack according to velocity (reverse, more velocity means a quicker attack)
		velocity = 1.f-velocity;
		m_voiceADSR.setTargetRatioA(kGoldenRatio + velocity*12.f);
		m_filterADSR.setTargetRatioA(0.5f*kGoldenRatio + velocity*6.f);

		m_voiceADSR.setAttackRate(attack);
		m_voiceADSR.setDecayRate(decay);
		m_voiceADSR.setReleaseRate(release);
		m_voiceADSR.setSustainLevel(sustain);

		m_filterADSR.setAttackRate(attack);
		m_filterADSR.setDecayRate(decay);
		m_filterADSR.setReleaseRate(release);
		m_filterADSR.setSustainLevel(0.25f + 0.75f*velocity);

		m_voiceADSR.gate(true);
		m_filterADSR.gate(true);
	}

	void ADSR::Stop(unsigned sampleCount, float velocity)
	{
		// Set decay & release according to velocity (aftertouch, less velocity means a slower release) 
		m_voiceADSR.setTargetRatioDR(kGoldenRatio + velocity*12.f);
		m_filterADSR.setTargetRatioDR(0.5f*kGoldenRatio + velocity*6.f);

		m_voiceADSR.gate(false);
		m_filterADSR.gate(false);
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
