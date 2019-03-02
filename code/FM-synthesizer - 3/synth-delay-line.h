
/*
	Syntherklaas FM -- Delay line (fractional).
*/

#pragma once

namespace SFM
{
	class DelayLine
	{
	public:
		DelayLine(size_t size);

		void Reset();

		SFM_INLINE void Write(float sample)
		{
			const unsigned index = m_writeIdx % m_size;
			m_buffer[index] = sample;
			++m_writeIdx;
		}

		// Delay is specified in samples relative to kSampleRate
		SFM_INLINE float Read(float delay)
		{
			const size_t from = (m_writeIdx-int(delay)) % m_size;
			const size_t to   = (from > 0) ? from-1 : m_size-1;
			const float fraction = fracf(delay);
			const float A = m_buffer[from];
			const float B = m_buffer[to];
			const float value = lerpf<float>(A, B, fraction);
			return value;
		}

		size_t size() const { return m_size; }

	private:
		const size_t m_size;
		unsigned m_writeIdx;

		alignas(16) float m_buffer[kSampleRate];
	};
}
