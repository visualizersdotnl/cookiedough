
/*
	Syntherklaas FM -- ADSR envelope.
*/

#include "synth-global.h"
#include "synth-ADSR.h"

namespace SFM
{
	void ADSR::Start(const Parameters &parameters, float velocity, float baseScale)
	{
		m_ADSR.reset();

		SFM_ASSERT(velocity <= 1.f);

		// 25% shorter attack, 75% shorter decay on max. velocity
		const float attackScale  = baseScale - 0.25f*velocity*baseScale;
		const float decayScale   = baseScale - 0.75f*velocity*baseScale;

		// Scale along with note velocity 
		const float releaseScale = baseScale * (1.f+velocity);
		
		// Attack & release have a minimum to prevent clicking
		const float attack  = floorf(attackScale*parameters.attack*kSampleRate);
		const float decay   = floorf(decayScale*parameters.decay*kSampleRate);
		const float release = std::max<float>(kSampleRate/1000.f /* 1ms min. avoids click */, floorf(releaseScale*parameters.release*kSampleRate));

		m_ADSR.setAttackRate(attack);
		m_ADSR.setDecayRate(decay);
		m_ADSR.setReleaseRate(release);
		m_ADSR.setSustainLevel(parameters.sustain);
		m_ADSR.setAttackLevel(parameters.attackLevel);

		m_ADSR.gate(true);
	}

	void ADSR::Stop(float velocity)
	{
		// FIXME: interpret high velocity (aftertouch) as a way to shorten the release phase?

		// Go into release state.
		m_ADSR.gate(false);
	}
}
