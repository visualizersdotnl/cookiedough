
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

		// 25% shorter attack & 100% longer release on max. velocity
		const float attackScale  = 1.f - 0.25f*velocity;
		const float releaseScale = 1.f + 2.f*velocity;
		
		// Attack & release have a minimum to prevent clicking
		const float attack  = floorf(attackScale*parameters.attack*kSampleRate);
		const float decay   = floorf(parameters.decay*kSampleRate);
		const float release = std::max<float>(kSampleRate/1000.f /* 1ms min. */, floorf(releaseScale*parameters.release*kSampleRate));
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
		const float invVel = 1.f-velocity;
		m_ADSR.setTargetRatioDR(invVel);

		// Go into release state.
		m_ADSR.gate(false);
	}
}
