
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
	private:
		unsigned m_sampleOffs;
		Waveform m_form;
		float m_amplitude;
		float m_frequency;
		float m_pitch;

	public:		
		LFO() : m_amplitude(0.f) {}

		void Initialize(unsigned sampleCount, Waveform form, float amplitude, float frequency);
		float Sample(unsigned sampleCount);
	};
}
