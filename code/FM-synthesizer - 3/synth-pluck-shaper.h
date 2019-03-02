
/*
	Syntherklaas FM -- Karplus-Strong-ish pluck used to guide distortion.

	Currently not in use!
*/

#pragma once

#include "synth-global.h"
#include "synth-stateless-oscillators.h"
#include "synth-one-pole-filters.h"

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
			
			// FIXME: use LUT
			float noise;
			for (unsigned iSample = 0; iSample < m_numSamples; ++iSample)
			{
				if (0 == (iSample & 15)) noise = oscWhiteNoise();
				m_buffer[iSample] = noise;
			}

			m_filter.SetCutoff(0.02f); // FIXME: parameter?
		}

		SFM_INLINE float Sample()
		{
			const float sample = m_buffer[m_index]; 
			const size_t next = (m_index+1) % m_buffer.size();
			const float lowpassed = m_filter.Apply(m_buffer[next]);
			m_buffer[m_index] = lowpassed;
			m_index = next;
			return sample;
		}

	private:
		/* const */ size_t m_numSamples;

		std::vector<float> m_buffer;
		size_t m_index;

		LowpassFilter m_filter;
	};
}
