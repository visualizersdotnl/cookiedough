
/*
	Syntherklaas FM -- Simple ring buffer (lockless) for samples.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	class FIFO
	{
	public:
		FIFO() :
			m_readIdx(0),
			m_writeIdx(0)
		{
			SFM_ASSERT(true == IsPow2(kRingBufferSize));
			memset(m_buffer, 0, kRingBufferSize*sizeof(float));
		}

		void Write(float value)
		{
			const unsigned index = m_writeIdx++ & (kRingBufferSize-1);
			m_buffer[index] = value;
		}

		float Read()
		{
			const unsigned index = m_readIdx++ & (kRingBufferSize-1);
			const float value = m_buffer[index];
			return value;
		}

		unsigned GetAvailable() const
		{
			return m_writeIdx-m_readIdx;
		}

		unsigned GetFree() const
		{
			return kRingBufferSize-GetAvailable();
		}
		
	private:
		unsigned m_readIdx;
		unsigned m_writeIdx;
		float m_buffer[kRingBufferSize];
	};
}
