
/*
	Syntherklaas FM -- ADSR envelope.
*/

namespace SFM
{
	struct ADSR
	{
		unsigned m_sampleOffs;
		float m_velocity;

		// In number of samples (must be within 1 second or max. kSampleRate)
		unsigned m_attack;
		unsigned m_decay;
		unsigned m_release;

		// Desired sustain [0..1]
		float m_sustain;

		// Note released?
		bool m_releasing;

		void Start(unsigned sampleCount, float velocity);
		void Stop(unsigned sampleCount);
		float Sample(unsigned sampleCount);	
	};
}
