
/*
	Syntherklaas FM - Pickup distortion.
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// FIXME: these numbers are of no particular significance
	const float kDefPickupDist = 1.1f;
	const float kDefPickupAsym = 0.3f;

	SFM_INLINE float fPickup(float signal, float distance, float asymmetry)
	{
		SampleAssert(signal);
		const float z = signal+asymmetry;
		return 1.f/(distance + z*z*z);
	}
}
