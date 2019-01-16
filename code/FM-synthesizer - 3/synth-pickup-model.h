
/*
	Syntherklaas FM - Pickup distortion.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	SFM_INLINE float fPickup(float signal, float distance, float asymmetry)
	{
		SampleAssert(signal);
		const float z = signal+asymmetry;
		return 1.f/(distance + z*z*z);
	}
}
