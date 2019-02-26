
/*
	Syntherklaas FM -- Delay line (fractional).
*/

#pragma once

namespace SFM
{
	const size_t kMaxDelayLineSize = kSampleRate;
	
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

		// FIXME: get rid of modulo
		SFM_INLINE float Read(float delay)
		{
			const int from = m_writeIdx-int(delay);
			const int to = from-1;
			const float fraction = fracf(delay);
			const float A = m_buffer[from % m_size];
			const float B = m_buffer[to   % m_size];
			const float value = lerpf<float>(A, B, fraction);
			return value;
		}

	private:
		/* const */ size_t m_size;
		unsigned m_writeIdx;

		alignas(16) float m_buffer[kMaxDelayLineSize];
	};
}
