
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

		const float shape = parameters.shape;
		SFM_ASSERT(shape >= 0.f && shape <= 1.f);

		float phase = 0.f;
		for (unsigned iSample = 0; iSample < kOscPeriod; ++iSample)
		{
			// Blend between raised cosine and another oscillator FM'ed by it
			const float cosine = powf(lutcosf(phase), parameters.curve);
			const float pulse = oscSoftSaw(phase + shape*cosine, 32);
			buffer[iSample] = lerpf<float>(cosine, pulse, shape);
			phase += pitch;
		}
	}

	void Modulator::Initialize(unsigned sampleOffs, float index, float frequency, float phaseShift, const IndexEnvelope::Parameters *pIndexEnvParams)
	{
		m_index = index;
		m_pitch = CalculateOscPitch(frequency);
		m_sampleOffs = sampleOffs;
		m_phaseShift = (phaseShift*kOscPeriod)/k2PI;

		if (nullptr != pIndexEnvParams)
			m_envelope.Calculate(*pIndexEnvParams);
	}

	float Modulator::Sample(unsigned sampleCount, float brightness)
	{
		const unsigned sample = sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch + m_phaseShift;

		brightness = invsqrf(brightness); 
		const float triangle = oscTriangle(phase);
		const float sine = oscSine(phase);
		const float modulation = sine + (triangle-sine)*brightness;
		const float index = m_index*LUTsample(m_envelope.buffer, phase);

		return index*modulation;
	}

	float Modulator::SimpleSample(unsigned sampleCount)
	{
		const unsigned sample = sampleCount-m_sampleOffs;
		const float phase = sample*m_pitch + m_phaseShift;

		const float sine = oscSine(phase);
		const float index = m_index;

		return index*sine;
	}
}
