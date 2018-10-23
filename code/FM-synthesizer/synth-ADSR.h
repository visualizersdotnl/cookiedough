
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
			// In number of samples (must be within 1 second or max. kSampleRate)
			unsigned attack;
			unsigned decay;
			unsigned release;

			// Desired sustain [0..1]
			float sustain;
		} m_parameters;

		unsigned m_sampleOffs;
		float m_curAmp;
		bool m_releasing;

		void Start(unsigned sampleCount, const Parameters &parameters, float velocity);
		void Stop(unsigned sampleCount);
		float Sample(unsigned sampleCount);	
	};
}
