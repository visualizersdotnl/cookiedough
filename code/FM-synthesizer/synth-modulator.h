
/*
	Syntherklaas FM -- Frequency modulator.

	This type of modulator is specific to FM. BISON.
*/

#pragma once

#include "synth-global.h"
#include "synth-oscillator.h"

namespace SFM
{
	class Modulator
	{
	private:
		float m_index;

		Oscillator m_oscSoft;
		Oscillator m_oscSharp;
		Oscillator m_indexLFO;

	public:		
		void Initialize(unsigned sampleCount, float index, float freqPM, float freqAM);
		float Sample(unsigned sampleCount, float brightness);
	};
}
