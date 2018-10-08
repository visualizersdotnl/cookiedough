

/*
	Syntherklaas FM -- 4-pole filter like the MOOG ladder.
*/

#ifndef _SFM_SYNTH_MOOG_LADDER_H_
#define _SFM_SYNTH_MOOG_LADDER_H_

#include "synth-global.h"

namespace SFM
{
	namespace MOOG
	{
		void SetCutoff(float cutoff);
		void SetResonance(float resonance); // Feed it [0..PI] (higher will cause sticky detuned feedback).
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
