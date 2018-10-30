
/*
	Syntherklaas FM -- Frequency modulator.

	This modulator has a brightness component which shifts between a sine and a triangle waveform
	and a frequency by which the index (or depth, if you like) can be modulated.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// To be initialized manually
	struct Modulator
	{
		unsigned m_sampleOffs;
		float m_index;
		float m_frequency;
		float m_pitch;
		float m_phaseShift;
		float m_indexModFreq;
		float m_indexModPitch;
		
		void Initialize(unsigned sampleCount, float index, float frequency, float phaseShift /* In kOscPeriod */, float indexModFreq);
		float Sample(unsigned sampleCount, float brightness);
	};
}
