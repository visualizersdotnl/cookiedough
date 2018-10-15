
/*
	Syntherklaas FM -- 4-pole MOOG ladder filter.

	A little quote about the self-oscillating resonance:
	
	"But just as you'd expect from a Moog filter, the Ladder self‑oscillates when the resonance control 
	is turned fully clockwise. This can be a disturbing effect at times, but it can be used to great advantage 
	in some situations. In other words, it's very much a feature rather than a bug, and demonstrates how 
	powerful this filter is; you simply have to know it and deal with it... and get some fun out of it! 
	In fact, with a little practice, you might even be able to coax some Theremin‑like sounds out of this tiny 
	module!"

	Source: https://www.soundonsound.com/reviews/moog-ladder
*/

#ifndef _SFM_SYNTH_MOOG_LADDER_H_
#define _SFM_SYNTH_MOOG_LADDER_H_

#include "synth-global.h"

namespace SFM
{
	namespace MOOG
	{
		// Witin normalied range [0..1000]
		void SetCutoff(float cutoff);
		
		// Feed it [0..PI], this just cuts off when it wants to start self-oscillating (see above)
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
