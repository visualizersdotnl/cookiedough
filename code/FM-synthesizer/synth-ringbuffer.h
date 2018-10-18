
/*
	Syntherklaas FM -- Simple ring buffer (lockless) for samples.

	FIXME: modify to use power of 2 size buffers only (and thus less costly modulo).
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	class FIFO
	{
	public:
		FIFO() :
			readIdx(0),
			writeIdx(0)
		{
			memset(buffer, 0, kRingBufferSize*sizeof(float));
		}

		void Write(float value)
		{
			buffer[writeIdx++ % kRingBufferSize] = value;
		}

		float Read()
		{
			const float value = buffer[readIdx++ % kRingBufferSize];
			return value;
		}

		unsigned GetAvailable() const
		{
			return writeIdx-readIdx;
		}

		unsigned GetFree() const
		{
			const unsigned maxWrite = kRingBufferSize-1;
			return std::min<unsigned>(maxWrite, maxWrite-GetAvailable());
		}
		
		unsigned readIdx;
		unsigned writeIdx;
		float buffer[kRingBufferSize];
	};
}
