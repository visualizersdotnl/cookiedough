
/*
	Syntherklaas FM -- ADSR envelope.
*/

#pragma once

//
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

		void Start(unsigned sampleCount, const Parameters &parameters, float velocity);
		void Stop(unsigned sampleCount);

		void Reset()
		{
			m_voiceADSR.reset();
		}

		float Sample(unsigned sampleCount)
		{
			return m_voiceADSR.process();
		}
		
		// Used to release voice to the pool
		bool IsReleased(unsigned sampleCount)
		{
			return m_voiceADSR.getState() == ::ADSR::env_idle;
		}
	};
}
