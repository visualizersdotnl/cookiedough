
/*
	Syntherklaas FM -- Frequency modulator.
*/

#include "synth-global.h"
#include "synth-modulator.h"
#include "synth-oscillators.h"

namespace SFM
{
	void Modulator::Initialize(unsigned sampleOffs, float index, float frequency, float phaseShift, float indexModFreq)
	{
		m_index = index;
		m_frequency = frequency;
		m_pitch = CalculateOscPitch(frequency);
		m_sampleOffs = sampleOffs;
		m_phaseShift = phaseShift;
		m_indexModFreq = indexModFreq;
		m_indexModPitch = CalculateOscPitch(indexModFreq);
	}

	float Modulator::Sample(unsigned sampleCount, float brightness)
	{
		const unsigned sample = sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch + m_phaseShift;
		const float modPhase = sample*m_indexModPitch;

		const float triangle = oscTriangle(phase);
		const float sine = lutsinf(phase);
		const float modulation = sine + (sine-triangle)*brightness;
		const float index = m_index*lutcosf(modPhase);

		return index*modulation;
	}
}
