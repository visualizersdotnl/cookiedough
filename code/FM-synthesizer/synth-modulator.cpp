
/*
	Syntherklaas FM -- Frequency modulator.
*/

#include "synth-global.h"
#include "synth-modulator.h"

namespace SFM
{
	void Modulator::Initialize(unsigned sampleCount, float index, float freqPM, float freqAM)
	{
		m_index = index;

		m_oscSoft.Initialize(sampleCount, kSine, freqPM, 1.f);
		m_oscSharp.Initialize(sampleCount, kPolyTriangle, freqPM, 1.f);
		m_indexLFO.Initialize(sampleCount, kCosine, freqAM, 1.f);
	}

	float Modulator::Sample(unsigned sampleCount, float brightness)
	{
		const float modulation = lerpf<float>(m_oscSoft.Sample(sampleCount, 0.f, 0.f), m_oscSharp.Sample(sampleCount, 0.f, 0.f), brightness);
		const float index = m_index*m_indexLFO.Sample(sampleCount, 0.f, 0.f);

		SampleAssert(modulation);
		FloatAssert(index);

		return index*modulation;
	}
}
