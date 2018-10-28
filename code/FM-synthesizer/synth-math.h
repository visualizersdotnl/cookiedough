
/*
	Syntherklaas FM -- Fast (approx.) math functions
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	const float kPI = 3.1415926535897932384626433832795f;
	const float kHalfPI = kPI*0.5f;
	const float k2PI = 2.f*kPI;
	const float kEpsilon = 5.96e-08f; // Max. error for single precision (32-bit).
	const float kGoldenRatio = 1.61803398875f;
	const float kRootHalf = 0.70710678118;

	// Fast approx. hyperbolic tangent function
	// Taken from: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=388650&sid=84cf3f5e70cec61f4b19c5031fe3a2d5
	SFM_INLINE float fast_tanhf(float x) 
	{
		const float ax = fabsf(x);
		const float x2 = x*x;
		const float z = x * (1.f + ax + (1.05622909486427f + 0.215166815390934f*x2*ax)*x2);
		return z/(1.02718982441289f + fabsf(z));
	}

	// "Ultra" version (claimed to be 2.5 times as precise)
	// Taken from: https://www.kvraudio.com/forum/viewtopic.php?f=33&t=388650&start=15
	SFM_INLINE float ultra_tanhf(float x)
	{
		const float ax = fabsf(x);
		const float x2 = x*x;
		const float z = x * (0.773062670268356f + ax + (0.757118539838817f + 0.0139332362248817f*x2*x2) * x2*ax);
		return z/(0.795956503022967f + fabsf(z));
	}

	// Bezier smoothstep
	SFM_INLINE float smoothstepf(float t)
	{
		return t*t * (3.f - 2.f*t);
	}

	// Simple lowpass
	SFM_INLINE float lowpassf(float from, float to, float factor)
	{
		SFM_ASSERT(factor >= 1.f);
		return ((from * (factor-1.f)) + to)/factor;
	}

	// Inverse square
	SFM_INLINE float invsqrf(float x)
	{
		x = 1.f-x;
		return 1.f - x*x;
	}

	// Scalar interpolation
	template<typename T>
	SFM_INLINE const T lerpf(const T &a, const T &b, float t)
	{
//		SFM_ASSERT(t >= -1.f && t <= 1.f);
		return a + (b-a)*t;
	}

	// GLSL frac()
	SFM_INLINE float fracf(float value) { return value - std::truncf(value); }
}
