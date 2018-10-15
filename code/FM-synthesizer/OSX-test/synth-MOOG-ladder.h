
/*
	Syntherklaas FM -- 4-pole MOOG ladder filter.
*/

#ifndef _SFM_SYNTH_MOOG_LADDER_H_
#define _SFM_SYNTH_MOOG_LADDER_H_

#include "synth-global.h"

namespace SFM
{
	namespace MOOG
	{
		// Witin normalied range [0..10000]
		void SetCutoff(float cutoff);
		
		// Feed it [0..PI], anything higher will cause it to detune and start self-oscillating
		// I suspect this is because I brought the precision down to 32-bit.
		// However, the paper also mentions that this implementation is closer to the original
		// filter's self-oscillating nature!
		void SetResonance(float resonance); 

		// FIXME: useful range to be determined
		void SetDrive(float drive);

		void ResetParameters();
		void ResetFeedback();

		SFM_INLINE void Reset()
		{
			ResetFeedback();
			ResetParameters();
		}

		void Filter(float *pDest, unsigned numSamples, float wetness);
	}
}

#endif // _SFM_SYNTH_MOOG_LADDER_H_
