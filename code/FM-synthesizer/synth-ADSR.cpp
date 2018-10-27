
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

		const float attack  = 1.f + truncf(parameters.attack*kSampleRate);
		const float decay   = 1.f + truncf(parameters.decay*kSampleRate);
		const float release = std::max<float>(kSampleRate/100.f /* 10ms. min */, truncf(parameters.release*kSampleRate));
		const float sustain = parameters.sustain;

		// I want more detail (ergo lesser ratio) if the velocity is higher, ergo:
		velocity = 1.f-velocity;

		// For now this sounds fine
		m_voiceADSR.setTargetRatioA(kGoldenRatio*2.f + velocity*0.624f);
		m_voiceADSR.setTargetRatioDR(kGoldenRatio*2.f + velocity*0.314f);

		// FIXME: tweak
		m_filterADSR.setTargetRatioA(0.3f);
		m_filterADSR.setTargetRatioDR(0.001f*kGoldenRatio + velocity*0.05f);

		m_voiceADSR.setAttackRate(attack);
		m_voiceADSR.setDecayRate(decay);
		m_voiceADSR.setReleaseRate(release);
		m_voiceADSR.setSustainLevel(sustain);

		m_filterADSR.setAttackRate(attack);
		m_filterADSR.setDecayRate(decay);
		m_filterADSR.setReleaseRate(release);
		m_filterADSR.setSustainLevel(sustain);

		m_voiceADSR.gate(true);
		m_filterADSR.gate(true);
	}

	void ADSR::Stop(unsigned sampleCount)
	{
		m_voiceADSR.gate(false);
		m_filterADSR.gate(false);
	}
}
