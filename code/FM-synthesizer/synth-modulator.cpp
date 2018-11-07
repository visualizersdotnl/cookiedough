
/*
	Syntherklaas FM -- Frequency modulator.
*/

#include "synth-global.h"
#include "synth-modulator.h"
#include "synth-oscillators.h"

namespace SFM
{
	void Modulator::Initialize(unsigned sampleCount, float index, float frequency, float indexModFreq)
	{
		m_sampleOffs = sampleCount;
		m_index = index;
		m_frequency = frequency;
		m_pitch = CalculatePitch(frequency);
		m_indexLFO.Initialize(sampleCount, kCosine, 1.f, indexModFreq);
	}

	float Modulator::Sample(unsigned sampleCount, float brightness)
	{
		const unsigned sample = sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch;
		
		const float triangle = oscDigiTriangle(phase);
		const float sine = lutsinf(phase);
		const float modulation = sine + (sine-triangle)*brightness;

		const float index = m_index*m_indexLFO.Sample(sampleCount);

		return index*modulation;
	}
}
