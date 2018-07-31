
// cookiedough -- utilities to port Shadertoy to CPU

/*
	FIXME:
		- add sign() function
		- add UV function that supplies offset and delta instead (only makes sense if dropping OpenMP)
*/

#pragma once

// SIMD versions of log(), exp(), sin(), cos()
#define USE_SSE2
#include "../3rdparty/sse_mathfun.h"

// blitters to output resolution
#include "map-blitter.h"
using namespace FXMAP;

namespace Shadertoy
{
	// -- constants copied from Std3DMath to promote their use, using these values often give *better* looking results --

	constexpr float kPI = 3.1415926535897932384626433832795f;
	constexpr float kHalfPI = kPI*0.5f;
	constexpr float k2PI = 2.f*kPI;
	constexpr float kEpsilon = 5.96e-08f; // Max. error for single precision (32-bit).
	constexpr float kGoldenRatio = 1.61803398875f;

	// refraction indices (you can do a Fresnel refraction using Std3DMath's Vector3)
	const float kVacuum = 0.f;
	const float kAir = 1.0003f;
	const float kWater = 1.3333f;
	const float kGlass = 1.5f;
	const float kPlastic = 1.5f;
	const float kDiamond = 2.417f;
 
	// -- math --

	VIZ_INLINE void rot2D(float angle, float &X, float &Y)
	{
		const int index = tocosindex(angle);
		const float cosine = lutcosf(index);
		const float sine = lutsinf(index);
		const float rotX = cosine*X - sine*Y;
		const float rotY = sine*X + cosine*Y;
		X = rotX;
		Y = rotY;
	}

	// -- UVs --

	VIZ_INLINE const Vector2 ToUV(unsigned iX, unsigned iY, float scale = 2.f)
	{
		float fX = (float) iX;
		float fY = (float) iY;
		fX *= 1.f/kResX;
		fY *= 1.f/kResY;
		return Vector2((fX-0.5f)*scale*kAspect, (fY-0.5f)*scale);
	}

	VIZ_INLINE const Vector2 ToUV_RT(unsigned iX, unsigned iY, float scale = 2.f)
	{
		float fX = (float) iX;
		float fY = (float) iY;
		fX *= 1.f/kTargetResX;
		fY *= 1.f/kTargetResY;
		return Vector2((fX-0.5f)*scale*kAspect, (fY-0.5f)*scale);
	}

	VIZ_INLINE const Vector2 ToUV_FX_2x2(unsigned iX, unsigned iY, float scale = 2.f)
	{
		float fX = (float) iX;
		float fY = (float) iY;
		fX *= 1.f/FXMAP::kFineResX;
		fY *= 1.f/FXMAP::kFineResY;
		return Vector2((fX-0.5f)*scale*kAspect, (fY-0.5f)*scale);
	}

	VIZ_INLINE const Vector2 ToUV_FX_4x4(unsigned iX, unsigned iY, float scale = 2.f)
	{
		float fX = (float) iX;
		float fY = (float) iY;
		fX *= 1.f/FXMAP::kCoarseResX;
		fY *= 1.f/FXMAP::kCoarseResY;
		return Vector2((fX-0.5f)*scale*kAspect, (fY-0.5f)*scale);
	}

	// -- color write --

	const __m128 chanScale = _mm_set1_ps(255.f);
//	const __m128 chanScale = _mm_set1_ps(1.f);

	// - writes 4 pixels at once
	// - assumes aligned input
	// - FIXME: swap R and B here?
	VIZ_INLINE __m128i ToPixel4(const Vector4 *colors)
	{
		__m128i zero = _mm_setzero_si128();

		// FIXME: shift instead of scale?
//		__m128i iA = _mm_max_epi32(zero, _mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[0].vSIMD)));
//		__m128i iB = _mm_max_epi32(zero, _mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[1].vSIMD)));
//		__m128i iC = _mm_max_epi32(zero, _mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[2].vSIMD)));
//		__m128i iD = _mm_max_epi32(zero, _mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[3].vSIMD)));

		__m128i iA = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[0].vSIMD)));
		__m128i iB = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[1].vSIMD)));
		__m128i iC = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[2].vSIMD)));
		__m128i iD = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[3].vSIMD)));

		__m128i AB = _mm_packus_epi32(iA, iB);
		__m128i CD = _mm_packus_epi32(iC, iD);
		return (_mm_packus_epi16(AB, CD));
	}

	// -- basic primitives --

	VIZ_INLINE float fSphere(const Vector3 &point, float radius)
	{
		return point.Length()-radius;
	}

	VIZ_INLINE float fBox(const Vector3 &point, const Vector3 size)
	{
		const float bX = std::max(0.f, fabsf(point.x)-size.x);
		const float bY = std::max(0.f, fabsf(point.y)-size.y);
		const float bZ = std::max(0.f, fabsf(point.z)-size.z);
		return sqrtf(bX*bX + bY*bY + bZ*bZ);
	}

	// -- ISSE color gamma-adjust-and-write (** also influences alpha, but that should not be a problem if you're not using it, or if it's 1.0 **)

	VIZ_INLINE __m128 GammaAdj(const Vector3 &color, float gamma = kGoldenRatio)
	{
		// raised = exp(exponent*log(value))
		return exp_ps(_mm_mul_ps(_mm_set1_ps(gamma), log_ps(color.vSIMD)));

	}
} 
