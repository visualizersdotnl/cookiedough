
/*
	Syntherklaas FM -- Delay line

	FIXME:
		- Offset support w/linear interpolation
		- Variable length driven by LFO
*/

#pragma once

#include "synth-modulator.h"
#include "synth-math.h"

namespace SFM
{
	class DelayLine
	{
	public:
		DelayLine() :
			m_readIdx(0)
,			m_writeIdx(0)
,			m_length(kSampleRate/8)
		{
			Reset();
		}

		void Reset()
		{
			memset(m_buffer, 0, kSampleRate*sizeof(float));
		}

		void Write(float sample, float feedback)
		{
			const unsigned index = m_writeIdx++ % m_length;
			m_buffer[index] = m_buffer[index]*feedback + sample;
		}

		float Read()
		{
			const unsigned index = m_readIdx++;
			return m_buffer[index % m_length];
		}
					
//		Modulator m_modulator;

	private:
		unsigned m_readIdx, m_writeIdx;
		unsigned m_length;

		float m_buffer[kSampleRate];
	};
}
