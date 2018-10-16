
/*
	Syntherklaas FM -- Mix(down) functions
*/

#ifndef _SFM_MIX_H_
#define _SFM_MIX_H_

namespace SFM
{
	SFM_INLINE float Clamp(float sample)
	{
		return clampf(-1.f, 1.f, sample);
	}

	// FIXME: LUT!
	SFM_INLINE float Crossfade(float dry, float wet, float time)
	{
		dry *= sqrtf(0.5f * (1.f+time));
		wet *= sqrtf(0.5f * (1.f-time));
		return Clamp(dry+wet);
	}
}

#endif