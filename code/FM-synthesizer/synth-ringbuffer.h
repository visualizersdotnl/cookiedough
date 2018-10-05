
/*
	Syntherklaas FM -- Simple ring buffer.
	FIXME: much faster if buffer size is power of 2, also untested!
*/

#ifndef _SFM_SYNTH_RINGBUFFER_H_
#define _SFM_SYNTH_RINGBUFFER_H_

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
			const float *pointer = buffer + (readIdx % kRingBufferSize);
			readIdx += numValues;
			return pointer;
		}

		unsigned GetAvail()
		{
			return writeIdx-readIdx;
		}
		
		unsigned readIdx;
		unsigned writeIdx;
		float buffer[kRingBufferSize];
	};
}

#endif // _SFM_SYNTH_RINGBUFFER_H_
