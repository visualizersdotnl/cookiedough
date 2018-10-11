
/*
	Syntherklaas FM -- OSX test Bevacaqua compatiblity (some of this should make it's way in officially).
*/

#ifndef _SFM_BEVACQUA_COMPAT_H_
#define _SFM_BEVACQUA_COMPAT_H_

// CRT & STL
#include <stdint.h>
#include <math.h>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <thread>
#include <math.h>
#include <assert.h>

#define VIZ_INLINE __inline
#define VIZ_ASSERT(condition) assert(condition)

/*
	Taken from Bevacqua's Std3DMath
*/

// A few meaningful constants.
constexpr float kPI = 3.1415926535897932384626433832795f;
constexpr float kHalfPI = kPI*0.5f;
constexpr float k2PI = 2.f*kPI;
constexpr float kEpsilon = 5.96e-08f; // Max. error for single precision (32-bit).
constexpr float kGoldenRatio = 1.61803398875f;

// GLSL-style clamp.
inline float clampf(float min, float max, float value)
{
	if (value < min)
		return min;

	if (value > max)
		return max;

	return value;
}

// HLSL saturate().
inline float saturatef(float value)
{
	return std::max<float>(0.f, std::min<float>(1.f, value));
}

// Scalar interpolation.
// FIXME: use multiply-add
template<typename T>
inline const T lerpf(const T &a, const T &b, float t)
{
	return a*(1.f-t) + b*t;
}

// Ken Perlin's take on Smoothstep.
// Source: http://en.wikipedia.org/wiki/Smoothstep
inline float smoothstepf(float a, float b, float t)
{
	t = saturatef(t);
	return lerpf<float>(a, b, t*t*t*(t*(t*6.f - 15.f) + 10.f));
}

/*
	Bevacqua's simpleton error mechanism
*/

__inline void SetLastError(const std::string &message) {}

#endif // _BEVACQUA_COMPAT_H_
