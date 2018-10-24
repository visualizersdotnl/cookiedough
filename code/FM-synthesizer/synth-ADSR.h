
/*
	Syntherklaas FM -- ADSR envelope.
*/

#pragma once

// Very nice implementation by Nigel Redmon (earlevel.com); using this for convenience and the fact that 
// with his experience it's modeled to behave like composers are likely used to
#include "3rdparty/ADSR.h"

namespace SFM
{
	struct ADSR
	{
		struct Parameters
		{
			// [0..1]
			float attack;
			float decay;
			float release;
			float sustain;
		};

		::ADSR m_voiceADSR;
		::ADSR m_filterADSR;

		void Start(unsigned sampleCount, const Parameters &parameters, float velocity);
		void Stop(unsigned sampleCount);

		void Reset()
		{
			m_voiceADSR.reset();
			m_filterADSR.reset();
		}

		float SampleForVoice(unsigned sampleCount)
		{
			return m_voiceADSR.process();
		}

		float SampleForFilter(unsigned sampleCount)
		{
			return m_filterADSR.process();
		}
		
		// Used to release voice to the pool
		bool IsReleased(unsigned sampleCount)
		{
			return m_voiceADSR.getState() == ::ADSR::env_idle;
		}
	};
}
