
/*
	Syntherklaas FM -- Fast (approx.) math functions
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	// Fast approx. hyperbolic tangent function
	// Taken from: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=388650&sid=84cf3f5e70cec61f4b19c5031fe3a2d5
	SFM_INLINE float fast_tanhf(float x) 
	{
		const float ax = fabsf(x);
		const float x2 = x*x;
		const float z = x * (1.f + ax + (1.05622909486427f + 0.215166815390934f*x2*ax)*x2);
		return z/(1.02718982441289f + fabsf(z));
	}

	// Bezier smoothstep
	SFM_INLINE float smoothstepf(float t)
	{
		return t*t * (3.f - 2.f*t);
	}
}
