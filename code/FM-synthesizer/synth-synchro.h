
/*
	Syntherklaas FM -- Oscillator phase-lock object
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	class Synchro
	{
	public:
		Synchro() : m_enabled(false) {}

		
		void Start(unsigned sampleCount, float frequency, float cycleLen)
		{
			m_enabled = true;
			m_sampleOffs = sampleCount;
			m_pitch = CalculatePitch(frequency);
		}

		void Stop()
		{
			m_enabled = false;
		}

		// Returns true when slaves should reset phase
		bool Tick(unsigned sampleCount)
		{
			if (true == m_enabled)
			{
				const unsigned sample = sampleCount-m_sampleOffs;
				const float phase = sample*m_pitch;
				if (phase >= m_cycleLen)
				{
					m_sampleOffs = sampleCount;
					return true;
				}
			}

			return false;
		}

	private:
		bool m_enabled;
		unsigned m_sampleOffs;
		float m_pitch;
		float m_cycleLen;
	};
}
