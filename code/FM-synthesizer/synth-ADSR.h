
/*
	Syntherklaas FM -- ADSR envelope.
*/

#pragma once

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
		} m_parameters;

		unsigned m_sampleOffs;

		unsigned m_attack;
		unsigned m_decay;
		unsigned m_release;
		float m_sustain;

		float m_curAmp;
		bool m_isReleasing;

		void Start(unsigned sampleCount, const Parameters &parameters, float velocity);
		void Stop(unsigned sampleCount);

		float Sample(unsigned sampleCount);
		
		// Used to release voice to the pool
		SFM_INLINE bool IsReleased(unsigned sampleCount)
		{
			return true == m_isReleasing && (m_sampleOffs+m_release < sampleCount);
		}
	};
}
