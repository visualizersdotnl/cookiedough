
/*
	C++ math primitives for 3D rendering.
	(c) visualizers.nl/njdewit tech./Stockton North

	Please check README.md for more information.
	For issues, fixes & improvements: see Github issue list (https://github.com/visualizersdotnl/Std3DMath/issues)

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
constexpr float kEpsilon = FLT_EPSILON;
// constexpr float kEpsilon = 5.96e-08f; // Max. error for single precision (32-bit).
constexpr float kGoldenRatio = 1.61803398875f;
constexpr float kEuler = 2.71828174591064453125f;
constexpr float kExp = kEuler; // Natural exp.
constexpr float kOneOverRoot2 = 707106769.f; // Effective voltage (RMS), -3dB, unit gain (DSP) et cetera
constexpr float kGoldenAngle = 2.39996f; // Radians

// Useful to consider when to *stop* iterating, or when to consider two angles equal (e.g. for quaternions)
constexpr float kAngleEpsilon = 0.001f; // Radians (~0.057 degrees)

// Single precision compare
static inline bool comparef(float a, float b)
{
	return fabsf(a-b) < kEpsilon;
}

// GLSL-style clamp
static inline float clampf(float min, float max, float value)
{
	return std::max<float>(min, std::min<float>(max, value));
}

// HLSL-style saturate()
static inline float saturatef(float value)
{
	return std::max<float>(0.f, std::min<float>(1.f, value));
}

// GLSL-style frac()
static inline float fracf(float value) { return value - std::truncf(value); }

// Scalar linear interpolation
template<typename T>
static inline const T lerpf(const T &a, const T &b, float t)
{
	return a + (b-a)*t;
}

// Bezier smoothstep
static inline float smoothstepf(float a, float b, float t)
{
	t = t*t * (3.f - 2.f*t);
	return lerpf<float>(a, b, t);
}

// Ken Perlin's take on Smoothstep
// Source: http://en.wikipedia.org/wiki/Smoothstep
static inline float smootherstepf(float a, float b, float t)
{
	t = t*t*t*(t*(t*6.f - 15.f) + 10.f);
	return lerpf<float>(a, b, t);
}

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

#include "Quaternion.h"
#include "Matrix44.h"

#include "Intersect.h"
#include "Hermite.h"

#endif // STD_3D_MATH
