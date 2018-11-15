
/*
	Syntherklaas FM -- Frequency modulator.

	This type of modulator needs to be replaced by Operator class(es).
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
		Oscillator m_indexLFO; // This one should probably bend as well (FIXME)

	public:
		// These are temporarily available for pitch bend
		Oscillator m_oscSoft;
		Oscillator m_oscSharp;

	public:		
		void Initialize(unsigned sampleCount, float index, float freqPM, float freqAM);
		float Sample(unsigned sampleCount, float brightness);
	};
}
