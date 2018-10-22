
/*
	Syntherklaas FM - Voice.
*/

#pragma once

#include "synth-global.h"
#include "synth-carrier.h"
#include "synth-modulator.h"
#include "synth-ADSR.h."

namespace SFM
{
	struct Voice
	{
		bool m_enabled;

		Carrier m_carrier;
		Modulator m_modulator;
		
		ADSR m_ADSR;

		float Sample(unsigned sampleCount)
		{
			const float modulation = m_modulator.Sample(sampleCount);
			const float ADSR = m_ADSR.Sample(sampleCount);
			const float sample = m_carrier.Sample(sampleCount, modulation);
			return ADSR*sample;
		}
	};
}
