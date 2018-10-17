
/*
	Syntherklaas FM -- Simple ring buffer (lockless).

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
			buffer[writeIdx % kRingBufferSize] = value;
			++writeIdx;
		}

		float *WritePtr(unsigned numValues)
		{
			SFM_ASSERT(GetFree() >= numValues);
			float *pointer = buffer + (writeIdx % kRingBufferSize);
			writeIdx += numValues;
			return pointer;
		}

		float Read()
		{
			const float value = buffer[readIdx % kRingBufferSize];
			++readIdx;
		}

		const float *ReadPtr(unsigned numValues)
		{
			SFM_ASSERT(GetAvail() >= numValues);
			const float *pointer = buffer + (readIdx % kRingBufferSize);
			readIdx += numValues;
			return pointer;
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
