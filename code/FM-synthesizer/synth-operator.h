
/*
	Syntherklaas FM - FM operator.
*/

#pragma once

#include "synth-global.h"
#include "synth-oscillator.h"
#include "synth-ADSR.h"

namespace SFM
{
	// DX7 style operator
	class Operator
	{
	private:
		Oscillator m_carrier;
		Oscillator m_LFO;
		ADSR m_envelope;

	public:
	};
}
