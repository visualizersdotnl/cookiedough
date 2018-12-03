
/*
	Syntherklaas FM -- ADSR envelope.
*/

#include "synth-global.h"
#include "synth-ADSR.h"

namespace SFM
{
	void ADSR::Start(const Parameters &parameters, float velocity, float freqScale /* = 0.f */)
	{
		m_ADSR.reset();

		SFM_ASSERT(velocity <= 1.f);

		// 50% shorter attack, 25% shorter decay on max. velocity
		const float attackScale  = 1.f -  0.5f*velocity;
		const float decayScale   = 1.f - 0.25f*velocity;

		// Expand release with respect to frequency (i.e. high notes have a shorter release, this makes physical sense)
		// FIXME: test!
		const float releaseScale = 1.f + 2.f*(velocity-freqScale);
		
		// Attack & release have a minimum to prevent clicking
		const float attack  = floorf(attackScale*parameters.attack*kSampleRate);
		const float decay   = floorf(decayScale*parameters.decay*kSampleRate);
		const float release = std::max<float>(kSampleRate/1000.f /* 1ms min. */, floorf(releaseScale*parameters.release*kSampleRate));
		const float sustain = parameters.sustain;

		// Harder touch, less linear
		const float invVel = 1.f-velocity;
		m_ADSR.setTargetRatioA(0.1f + invVel);
		m_ADSR.setTargetRatioDR(0.1f + invVel);

		m_ADSR.setAttackRate(attack);
		m_ADSR.setDecayRate(decay);
		m_ADSR.setReleaseRate(release);
		m_ADSR.setSustainLevel(sustain);

		m_ADSR.gate(true);
	}

	void ADSR::Stop(float velocity)
	{
		// Harder touch, less linear
		// Not factoring in frequency here; elongating the release cycle (above) should be sufficient
		const float invVel = 1.f-velocity;
		m_ADSR.setTargetRatioDR(0.1f + invVel);

		// Go into release state.
		m_ADSR.gate(false);
	}
}
