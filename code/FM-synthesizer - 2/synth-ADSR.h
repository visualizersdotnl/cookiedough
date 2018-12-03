
/*
	Syntherklaas FM -- ADSR envelope.
*/

#pragma once

// Nice implementation by Nigel Redmon (earlevel.com)
#include "3rdparty/ADSR.h"

namespace SFM
{
	class ADSR
	{
	public:
		ADSR()
		{
			Reset();
		}

		struct Parameters
		{
			// [0..1]
			float attack;
			float decay;
			float release;
			float sustain;
		};

		::ADSR m_ADSR;

		void Start(const Parameters &parameters, float velocity, float freqScale = 0.f);
		void Stop(float velocity /* Aftertouch? */);

		void Reset()
		{
			m_ADSR.reset();
		}

		float Sample()
		{
			return m_ADSR.process();
		}

		bool IsIdle() /* const */
		{
			const bool isIdle = m_ADSR.getState() == ::ADSR::env_idle;
			SFM_ASSERT(false == isIdle || (true == isIdle && 0.f == m_ADSR.getOutput()));
			return m_ADSR.getState() == ::ADSR::env_idle;
		}
	};
}
