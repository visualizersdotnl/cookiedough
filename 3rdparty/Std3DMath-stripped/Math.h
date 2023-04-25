
/*
	C++ math primitives for 3D rendering.
	(c) visualizers.nl

	Please check README.md for more information.

	Added, fixed and/or modified (backport to main branch):
	- Fixed issue raised by Marco Foco (see for ex. Vector3::Add()).
	- Added Matrix44::FromArray33().
	- Forced 16-byte alignment for Vector4 & Vector3 (padded) by unionizing with __m128 (SSE/SIMD).
	- kPI et cetera are now 'constexpr'.
	- I'm dead tired of MSVC not inlining what it could and should, so I'm going to force a few, look for S3D_INLINE.
	- Removed empty constructors (put it back for Vector3) & destructors, except for Vector4.
	- Added "2*kPI".
	- Added an actual Multiplied()/Multiply() function on Vector3 and Vector4 as I really needed it more often.
	- Fix: used Scale() function instead of Mul() when multiplying vector by scalar.
	- Added fracf().
	- Fixed clampf().
	- Fixed lerpf(), smoothstepf() & smootherstepf().

	Added after integrating fixes on 19/10/2018:
	- Little typo in smoothstepf().
	- Added weighted average.

	Pay attention to:
	- Added cast operator (const) to __m128 on Vector3/Vector4 (don't backport, or do it in a portable fashion).
	- Problem: lerpf() doesn't work on vector types because they do a dot() when using the asterisk operator.
*/

#if !defined(STD_3D_MATH)
#define STD_3D_MATH

#include "Dependencies.h"

// A few meaningful constants.
constexpr float kPI = 3.1415926535897932384626433832795f;
constexpr float kHalfPI = kPI*0.5f;
constexpr float k2PI = 2.f*kPI;
constexpr float kEpsilon = 5.96e-08f; // Max. error for single precision (32-bit).
constexpr float kGoldenRatio = 1.61803398875f;

// Generic floating point random.
// Has poor distribution due to rand() being 16-bit, so don't use it when proper distribution counts.
static inline float randf(float range)
{
	return range*((float) rand() / float(RAND_MAX));
}

// Single precision compare.
static inline bool comparef(float a, float b)
{
	return fabsf(a-b) < kEpsilon;
}

// GLSL-style clamp.
static inline float clampf(float min, float max, float value)
{
	if (value < min)
		return min;

	if (value > max)
		return max;

	return value;
}

// HLSL saturate().
static inline float saturatef(float value)
{
	return std::max<float>(0.f, std::min<float>(1.f, value));
}

// GLSL fract().
static inline float fracf(float value) { return value - ::floorf(value); }

// Scalar interpolation.
template<typename T>
static inline const T lerpf(const T &a, const T &b, float t)
{
	return a + (b-a)*t;
}

// Bezier smoothstep.
static inline float smoothstepf(float a, float b, float t)
{
	t = t*t * (3.f - 2.f*t);
	return lerpf<float>(a, b, t);
}

// Ken Perlin's take on Smoothstep.
// Source: http://en.wikipedia.org/wiki/Smoothstep
static inline float smootherstepf(float a, float b, float t)
{
	t = t*t*t*(t*(t * 6.f-15.f) + 10.f);
	return lerpf<float>(a, b, t);
}

// Weighted average (type of low-pass).
static inline float lowpassf(float from, float to, float factor)
{
	return ((from * (factor-1.f)) + to)/factor;
}


#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Quaternion.h"
#include "Matrix44.h"

#endif // STD_3D_MATH
