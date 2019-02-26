
/*
	Syntherklaas FM -- ADSR envelope.

	For information on it's shape & target ratios: https://youtu.be/0oreYmOWgYE
*/

#include "synth-global.h"
#include "synth-ADSR.h"

namespace SFM
{
	void ADSR::Start(const Parameters &parameters, float velocity, float baseScale)
	{
		m_ADSR.reset();

		SFM_ASSERT(velocity <= 1.f);

		// 25% shorter attack @ max. velocity
		const float attackScale  = baseScale - 0.25f*velocity*baseScale;

		// 25% longer decay @ max. velocity
		const float decayScale   = baseScale + 0.25f*velocity*baseScale;

		// Scale along with note velocity 
		const float releaseScale = baseScale * (1.f+velocity);
		
		// Attack & release have a minimum to prevent clicking
		const float attack  = std::max<float>(kSampleRate*0.001f /* 1ms min. */, floorf(attackScale*parameters.attack*kSampleRate));
		const float decay   = floorf(decayScale*parameters.decay*kSampleRate);
		const float release = std::max<float>(kSampleRate*0.001f /* 1ms min. */, floorf(releaseScale*parameters.release*kSampleRate));

		m_ADSR.setAttackRate(attack);
		m_ADSR.setDecayRate(decay);
		m_ADSR.setReleaseRate(release);

		m_ADSR.setAttackLevel(parameters.attackLevel);
		m_ADSR.setSustainLevel(parameters.sustain);

		const float linearity = 1.f; // FIXME: parameter
		const float defRatioA = 0.3f;
		const float defRatioDR = 0.0001f;
		const float ratioA = lerpf(defRatioA, defRatioA+100.f, linearity);
		const float ratioDR = lerpf(defRatioDR, defRatioDR+100.f, linearity);

		m_ADSR.setTargetRatioA(ratioA);
		m_ADSR.setTargetRatioDR(ratioDR);

		m_ADSR.gate(true);
	}

	void ADSR::Stop(float velocity)
	{
		// FIXME: interpret high velocity (aftertouch) as a way to shorten the release phase?

		// Go into release state.
		m_ADSR.gate(false);
	}
}
