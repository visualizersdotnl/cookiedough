
/*
	Syntherklaas FM -- Bipolar AD(S) envelope (FIXME).
*/

#pragma once

namespace SFM
{
	class ADS
	{
	private:
		enum State
		{
			kIdle,
			kAttack,
			kDecay
		}  m_state;

	public:
		ADS()
		{
			Reset();
		}

		void Start(float attack, float decay, float velocity);

		void Reset()
		{
			m_state = kIdle;
		}

		float Sample()
		{
		}
	};
}
