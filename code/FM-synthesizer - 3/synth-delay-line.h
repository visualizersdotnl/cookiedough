
/*
	Syntherklaas FM -- Delay line (fractional).
*/

#pragma once

#include "synth-global.h"

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
			const int to = m_writeIdx-int(delay);
			const int from = to-1;
			const float fraction = fracf(delay);
			
			// FIXME: eradicate modulos
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
