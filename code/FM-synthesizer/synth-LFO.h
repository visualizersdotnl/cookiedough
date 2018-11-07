
/*
	Syntherklaas FM -- Simple continuous oscillator (LFO).

	Only supports a limited set of waveforms (see implementation).
*/

#pragma once

#include "synth-global.h"
#include "synth-stateless-oscillators.h"

namespace SFM
{
	class LFO
	{
	public:
		LFO() : m_enabled(false) {}

	private:
		bool m_enabled;
	
		unsigned m_sampleOffs;
		Waveform m_form;
		float m_depth;
		float m_frequency;
		float m_pitch;

	public:		
		void Initialize(unsigned sampleCount, Waveform form, float depth, float frequency);
		float Sample(unsigned sampleCount);

		void Stop()
		{
			SFM_ASSERT(true == m_enabled);
			m_enabled = false;
		}

		void Restart(unsigned sampleCount)
		{
			SFM_ASSERT(false == m_enabled);
			m_enabled = true;

			m_sampleOffs = sampleCount;
		}

		bool IsEnabled() const     { return m_enabled;   }
		float GetFrequency() const { return m_frequency; }
	};
}
