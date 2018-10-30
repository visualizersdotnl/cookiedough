
/*
	Syntherklaas FM -- ADSR envelope.

	The ADSR class uses Nigel Redmon's implementation; the ADSR_Simple is my basic test implementation.
	I'm keeping the latter for testing purposes.
*/

#pragma once

// Very nice implementation by Nigel Redmon (earlevel.com); using this for convenience and the fact that 
// with his experience it's modeled to behave like composers are likely used to
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

		unsigned m_sampleOffs;

		::ADSR m_voiceADSR;
		::ADSR m_filterADSR;

		void Start(unsigned sampleCount, const Parameters &parameters, float velocity);
		void Stop(unsigned sampleCount, float velocity);

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
		
		bool IsIdle(unsigned sampleCount) /* const */
		{
			const bool isIdle = m_voiceADSR.getState() == ::ADSR::env_idle;
			SFM_ASSERT(false == isIdle || (true == isIdle && 0.f == m_voiceADSR.getOutput()));
			return m_voiceADSR.getState() == ::ADSR::env_idle;
		}
	};

	/*
		ADSR_Simple
	*/

	class ADSR_Simple
	{
	public:
		ADSR_Simple() : 
			m_state(kIdle),
			m_output(0.f)
		{}

		struct Parameters
		{
			// [0..1]
			float attack;
			float decay;
			float release;
			float sustain;
		};

		unsigned m_sampleOffs;

		enum State
		{
			kAttack,
			kDecay,
			kSustain,
			kRelease,
			kIdle
		} m_state;

		float m_output;

		unsigned m_attack;
		unsigned m_decay;
		unsigned m_release;
		float    m_sustain;

		void Start(unsigned sampleCount, const Parameters &parameters, float velocity);
		void Stop(unsigned sampleCount);

		void Reset();

		float SampleForVoice(unsigned sampleCount);
		float SampleForFilter(unsigned sampleCount) { return 1.f; } // FIXME
		
		bool IsIdle(unsigned sampleCount) const
		{
			return kIdle == m_state;
		}
	};
}
