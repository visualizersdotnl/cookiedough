
/*
	Syntherklaas FM -- Frequency modulator (can also be used as LFO).
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	struct Modulator
	{
		float m_index;
		float m_pitch;
		unsigned m_sampleOffs;
		float m_phaseShift;
		
		// Currently for a 'cosine tilt' envelope
		float m_envelope[kOscPeriod];

		void Initialize(unsigned sampleCount, float index, float frequency, float phaseShift /* In radians */);
		float Sample(unsigned sampleCount, const float *pLFO);
	};
}
