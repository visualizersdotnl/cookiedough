
/*
	Syntherklaas FM -- Frequency modulator (can also be used as LFO).
*/

#include "synth-global.h"
#include "synth-modulator.h"
#include "synth-oscillators.h"

namespace SFM
{
	void IndexEnvelope::Calculate(const Parameters &parameters)
	{
		const float pitch = CalculateOscPitch(parameters.frequency);

		const float tilt = parameters.tilt;
		SFM_ASSERT(tilt >= 0.f && tilt <= 1.f);

		float phase = 0.f;
		for (unsigned iSample = 0; iSample < kOscPeriod; ++iSample)
		{
			// Blend between raised cosine and a square FM'ed by it
			const float cosine = powf(lutcosf(phase), parameters.curve);
			const float pulse = oscTriangle(phase+cosine);
			buffer[iSample] = lerpf<float>(cosine, pulse, tilt*tilt);
			phase += pitch;
		}
	}

	void Modulator::Initialize(unsigned sampleOffs, float index, float frequency, float phaseShift, const IndexEnvelope::Parameters &indexEnvParams)
	{
		m_index = index;
		m_pitch = CalculateOscPitch(frequency);
		m_sampleOffs = sampleOffs;
		m_phaseShift = (phaseShift*kOscPeriod)/k2PI;
		m_envelope.Calculate(indexEnvParams);
	}

	float Modulator::Sample(unsigned sampleCount)
	{
		const unsigned sample = sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch + m_phaseShift;

		// FIXME: try other oscillators (not without risk of noise, of course)
		const float modulation = oscSine(phase); 
		const float index = m_index*LUTsample(m_envelope.buffer, phase);

		return index*modulation;
	}
}
