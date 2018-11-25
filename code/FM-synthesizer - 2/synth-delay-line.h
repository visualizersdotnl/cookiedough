
/*
	Syntherklaas FM -- Delay line (fractional).
*/

#pragma once

namespace SFM
{
	const unsigned kDelayLineSize = kSampleRate;
	
	class DelayLine
	{
	public:
		DelayLine() :
			m_writeIdx(0)
		{
			Reset();
		}

		void Reset()
		{
			memset(m_buffer, 0, kDelayLineSize*sizeof(float));
		}

		SFM_INLINE void Write(float sample)
		{
			const unsigned index = m_writeIdx % kDelayLineSize;
			m_buffer[index] = sample;
			++m_writeIdx;
		}

		SFM_INLINE float Read(float delay)
		{
			SFM_ASSERT(delay >= 0.f && delay <= 1.f);
			delay *= kDelayLineSize;
				
			const float fraction = fracf(delay);
			const int from = m_writeIdx-int(delay)-1;
			const int to = from-1;

			const float A = m_buffer[from % kDelayLineSize];
			const float B = m_buffer[to   % kDelayLineSize];
			const float value = lerpf<float>(A, B, fraction);

			return value;
		}

	private:
		unsigned m_writeIdx;

		alignas(16) float m_buffer[kDelayLineSize];
	};
}
