
/*
	Syntherklaas FM -- Simple ring buffer (lockless) for audio streaming.
*/

#pragma once

namespace SFM
{
	class RingBuffer
	{
	public:
		RingBuffer() :
			m_readIdx(0),
			m_writeIdx(0)
		{
			SFM_ASSERT(true == IsPow2(kRingBufferSize));
			memset(m_buffer, 0, kRingBufferSize*sizeof(float));
		}

		SFM_INLINE void Write(float value)
		{
			m_buffer[m_writeIdx & (kRingBufferSize-1)] = value;
			m_writeIdx = ++m_writeIdx;
		}

		SFM_INLINE float Read()
		{
			SFM_ASSERT(m_readIdx < m_writeIdx); // Underrun
			const float value = m_buffer[m_readIdx & (kRingBufferSize-1)];
			++m_readIdx;
			return value;
		}

		void Flush(float *pDest, unsigned numElements)
		{
			SFM_ASSERT(numElements <= kRingBufferSize);
//			SFM_ASSERT(m_readIdx < m_writeIdx); // Underrun (FIXME: kept happening too much in debug builds)
			const unsigned range = m_readIdx+numElements;
			const unsigned head = range & (kRingBufferSize-1);
			const unsigned tail = numElements-head;
			memcpy(pDest, m_buffer+m_readIdx, head*sizeof(float));
			memcpy(pDest+head, m_buffer, tail*sizeof(float));
			m_readIdx = range;
		}

		unsigned GetAvailable() const
		{
			return m_writeIdx-m_readIdx;
		}

		bool IsFull() const
		{
			return m_writeIdx == m_readIdx+kRingBufferSize;
		}
		
	private:
		unsigned m_readIdx;
		unsigned m_writeIdx;

		alignas(16) float m_buffer[kRingBufferSize];
	};
}
	