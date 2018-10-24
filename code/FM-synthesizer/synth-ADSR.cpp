
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
//		m_voiceADSR.reset();

		const float attack  = 1.f +   truncf(parameters.attack*kSampleRate);
		const float decay   = 1.f +   truncf(parameters.decay*kSampleRate);
		const float release = 128.f + truncf(parameters.release*kSampleRate);
		const float sustain = parameters.sustain;

		// For now this sounds fine
		m_voiceADSR.setTargetRatioA(kGoldenRatio*2.f - velocity*0.6f);
		m_voiceADSR.setTargetRatioDR(kGoldenRatio*2.f - velocity*0.6f);

		m_voiceADSR.setAttackRate(attack);
		m_voiceADSR.setDecayRate(decay);
		m_voiceADSR.setReleaseRate(release);
		m_voiceADSR.setSustainLevel(sustain);

		m_voiceADSR.gate(true);
	}

	void ADSR::Stop(unsigned sampleCount)
	{
		m_voiceADSR.gate(false);
	}
}
