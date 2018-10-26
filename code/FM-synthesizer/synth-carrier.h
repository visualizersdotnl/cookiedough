
/*
	Syntherklaas FM -- Carrier wave.
*/

#pragma once

#include "synth-oscillators.h"

namespace SFM
{
	struct Carrier
	{
		Waveform m_form;
		float m_amplitude;
		float m_frequency;
		float m_pitch;
		unsigned m_sampleOffs;
		unsigned m_numHarmonics;

		void Initialize(unsigned sampleCount, Waveform form, float amplitude, float frequency);
		float Sample(unsigned sampleCount, float modulation);
	};
}
