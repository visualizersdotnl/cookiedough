
/*
	Syntherklaas FM -- ADSR envelope.

	For information on it's shape & target ratios: https://youtu.be/0oreYmOWgYE
*/

#include "synth-global.h"
#include "synth-ADSR.h"

namespace SFM
{
	void ADSR::Start(const Parameters &parameters, float velocity, float baseScale, float linearity)
	{
		m_ADSR.reset();

		// Bounds check
		SFM_ASSERT(velocity >= 0.f && velocity <= 1.f);
		SFM_ASSERT(baseScale >= 0.f);
		SFM_ASSERT(linearity >= 0.f && linearity <= 1.f);

		// Stretch response between Nigel's sensible analog-style defaults and something way more linear
		const float defRatioA = 0.3f;
		const float defRatioDR = 0.0001f;
		const float ratioA = defRatioA + linearity*10.f;
		const float ratioDR = defRatioDR + linearity*10.f;

		m_ADSR.setTargetRatioA(ratioA);
		m_ADSR.setTargetRatioDR(ratioDR);

		// The following is purely based on instruments like guitar or piano, so *maybe* I should do away with it? (FIXME)
		// - 30% shorter attack @ max. velocity
		// - 30% longer decay @ max. velocity
		// - 30% longer release @ max. velocity
		const float stretch      = 0.3f*velocity*baseScale;
		const float attackScale  = baseScale-stretch;
		const float decayScale   = baseScale+stretch;
		const float releaseScale = baseScale+stretch;
		
		// Attack & release have a minimum to prevent clicking
		const float attack  = std::max<float>(kSampleRate*0.001f /* 1ms min. */, floorf(attackScale*parameters.attack*kSampleRate));
		const float decay   = floorf(decayScale*parameters.decay*kSampleRate);
		const float release = std::max<float>(kSampleRate*0.001f /* 1ms min. */, floorf(releaseScale*parameters.release*kSampleRate));

		m_ADSR.setAttackRate(attack);
		m_ADSR.setDecayRate(decay);
		m_ADSR.setReleaseRate(release);

		m_ADSR.setAttackLevel(parameters.attackLevel);
		m_ADSR.setSustainLevel(parameters.sustain);

		m_ADSR.gate(true);
	}

	void ADSR::Stop(float velocity)
	{
		// FIXME: interpret high velocity (aftertouch) as a way to shorten the release phase?

		// Go into release state.
		m_ADSR.gate(false);
	}
}
