
/*
	Syntherklaas FM -- Delay line.
*/

#pragma once

namespace SFM
{
	class DelayLine
	{
	public:
		DelayLine(unsigned length) :
			m_readIdx(0)
,			m_writeIdx(0)
,			m_length(length)
		{
			SFM_ASSERT(length > 0 && length <= kSampleRate);
			Reset();
		}

		void Reset()
		{
			memset(m_buffer, 0, kSampleRate*sizeof(float));
		}

		SFM_INLINE void Write(float sample)
		{
			const unsigned index = m_writeIdx % m_length;
			m_buffer[index] = sample;
			++m_writeIdx;
		}

		SFM_INLINE float Tap(float delay)
		{
			int from = int(delay);
			const float fraction = delay-from;
			from = m_readIdx-1-from;
			const int to = from-1;
			const float A = m_buffer[from % m_length];
			const float B = m_buffer[to   % m_length];
			const float value = lerpf<float>(A, B, fraction);
			return value;
		}

		SFM_INLINE void Next()
		{
			++m_readIdx;
		}
			
	private:
		unsigned m_readIdx, m_writeIdx;
		unsigned m_length;

		alignas(16) float m_buffer[kSampleRate];
	};
}
