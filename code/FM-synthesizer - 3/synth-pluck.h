
/*
	Syntherklaas FM -- Karplus-Strong pluck.

	FIXME: *very* much a prototype
*/

#pragma once

#include "synth-global.h"
#include "synth-stateless-oscillators.h"
#include "synth-one-pole.h"
// #include "synth-delay-line.h"

namespace SFM
{
	class Pluck
	{
	public:
		Pluck() {}

		Pluck(float frequency) :
			m_numSamples(size_t(kSampleRate/frequency))
,			m_index(0)
		{
			SFM_ASSERT(0.f != frequency);

			m_buffer.resize(m_numSamples);
			
			// FIXME: precalculate larger table and pick random offset
			LowpassFilter filter;
			filter.SetCutoff(0.1f);
			for (unsigned iSample = 0; iSample < m_numSamples; ++iSample)
			{
				const float sample = filter.Apply(oscWhiteNoise());
				m_buffer[iSample] = sample;
			}
		}

		SFM_INLINE float Sample()
		{
			const float sample = m_buffer[m_index];
			const size_t next = (m_index+1) % m_buffer.size();
			const float lowpassed = 0.5f*(sample+m_buffer[next])*0.996f; // FIXME: variable!
			m_buffer[m_index] = lowpassed;
			m_index = next;
			return sample;
		}

	private:
		/* const */ size_t m_numSamples;

		std::vector<float> m_buffer; // FIXME: use delay line
		size_t m_index;
	};
}
