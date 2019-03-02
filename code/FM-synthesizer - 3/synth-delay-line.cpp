
/*
	Syntherklaas FM -- Delay line (fractional).
*/

#pragma once

#include "synth-global.h"
#include "synth-delay-line.h"

namespace SFM
{
	DelayLine::DelayLine(size_t size) :
		m_size(size)
,		m_writeIdx(0)
	{
		SFM_ASSERT(size <= kSampleRate);
		Reset();
	}

	void DelayLine::Reset()
	{
		memset(m_buffer, 0, m_size*sizeof(float));
	}
}
