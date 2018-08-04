
// cookiedough -- a (messy) collection of values and functions to easily implement Shadertoy(ish) effects

/*
	- the rule of thumb now is that once you go to __m128 (color operations, mostly) you don't convert back
	- keep an eye on performance when employing SIMD, it's not always faster

	FIXME:
		- add sign() function
		- add 4-tap normal
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

	// vector interpolate - use this instead of lerpf() (which, see /3rdparty/Std3DMath/Math.h, does not work as intended)
	VIZ_INLINE __m128 vLerp4(__m128 A, __m128 B, float factor)
	{
		const __m128 alphaUnp = _mm_set1_ps(factor);
		const __m128 delta = _mm_mul_ps(alphaUnp, _mm_sub_ps(B, A));
		return _mm_add_ps(A, delta);
	}

	VIZ_INLINE float vFastLen3(const Vector3 &vector)
	{
		return 1.f/Q3_rsqrtf(vector.x*vector.x + vector.y*vector.y + vector.z*vector.z);
	}

	// use this instead of Normalize()/Normalized() on Vector3
	VIZ_INLINE void vFastNorm3(Vector3 &vector)
	{
		vector *= Q3_rsqrtf(vector.x*vector.x + vector.y*vector.y + vector.z*vector.z);
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

	// - writes 4 pixels at once
	// - FIXME: swap R and B here?
	VIZ_INLINE __m128i ToPixel4(const __m128 *colors)
	{
		const __m128i iA = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[0])));
		const __m128i iB = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[1])));
		const __m128i iC = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[2])));
		const __m128i AB = _mm_packus_epi32(iA, iB);
		const __m128i iD = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[3])));
		const __m128i CD = _mm_packus_epi32(iC, iD);

		return (_mm_packus_epi16(AB, CD));
	}

	VIZ_INLINE __m128i ToPixel4_NoConv(const __m128 *colors)
	{
		const __m128i iA = _mm_cvtps_epi32(colors[0]);
		const __m128i iB = _mm_cvtps_epi32(colors[1]);
		const __m128i iC = _mm_cvtps_epi32(colors[2]);
		const __m128i AB = _mm_packus_epi32(iA, iB);
		const __m128i iD = _mm_cvtps_epi32(colors[3]);
		const __m128i CD = _mm_packus_epi32(iC, iD);

		return (_mm_packus_epi16(AB, CD));
	}

	// -- basic primitives --

	VIZ_INLINE float fSphere(const Vector3 &point, float radius)
	{
		return vFastLen3(point)-radius; // point.Length()-radius;
	}

	// FIXME: use fast inverse square root!
	VIZ_INLINE float fBox(const Vector3 &point, const Vector3 size)
	{
		const float bX = std::max(0.f, fabsf(point.x)-size.x);
		const float bY = std::max(0.f, fabsf(point.y)-size.y);
		const float bZ = std::max(0.f, fabsf(point.z)-size.z);
		return sqrtf(bX*bX + bY*bY + bZ*bZ);
	}

	// -- colorization (mostly takes __m128 and doesn't go back, the rest: FIXME!) --

	// SIMD color gamma-adjust-and-write (** also influences alpha, but that should not be a problem if you're not using it, or if it's 1.0 **)
	VIZ_INLINE __m128 GammaAdj(__m128 color, float gamma = kGoldenRatio)
	{
		// formula: raised = exp(exponent*log(value))
		return exp_ps(_mm_mul_ps(_mm_set1_ps(gamma), log_ps(color)));
	}

	// IQ's palette function
	VIZ_INLINE const __m128 CosPal(float index, const Vector3 &bias, const Vector3 &scale, const Vector3 &frequency, const Vector3 &phase)
	{
		const __m128 vIndex = _mm_set1_ps(index);
		const __m128 v2PI = _mm_set1_ps(k2PI);
		const __m128 angles = _mm_mul_ps(_mm_add_ps(_mm_mul_ps(frequency, vIndex), phase), v2PI);
		const __m128 cosines = cos_ps(angles);
		return _mm_add_ps(bias, _mm_mul_ps(cosines, scale));

//		Vector3 cosines = (frequency*index + phase)*k2PI;
//		cosines.x = lutcosf(cosines.x);
//		cosines.y = lutcosf(cosines.y);
//		cosines.z = lutcosf(cosines.z);
//		return bias+cosines.Multiplied(scale);
	}

	// IQ's palette function (simplified)
	VIZ_INLINE const Vector3 CosPalSimple(float index, const Vector3 &bias, float cosScale, const Vector3 &frequency, float phase)
	{
		Vector3 cosines = (frequency*index + phase)*k2PI;
		cosines.x = lutcosf(cosines.x);
		cosines.y = lutcosf(cosines.y);
		cosines.z = lutcosf(cosines.z);
		return bias + cosines*cosScale;
	}

	// IQ's palette function (simplified)
	VIZ_INLINE const Vector3 CosPalSimplest(float index, float cosScale, const Vector3 &frequency, float phase)
	{
		Vector3 cosines = (frequency*index + phase)*k2PI;
		cosines.x = lutcosf(cosines.x);
		cosines.y = lutcosf(cosines.y);
		cosines.z = lutcosf(cosines.z);
		return cosines*cosScale;
	}

	// exponential fog
	VIZ_INLINE __m128 ApplyFog(float distance, __m128 color, __m128 fogColor, float scale = 0.004f)
	{
		const float fog = 1.f-(expf(-scale*distance*distance));
		return Shadertoy::vLerp4(color, fogColor, fog);
	}

	// cheap desaturation (amount [0..1])
	VIZ_INLINE __m128 Desaturate(__m128 color, float amount)
	{
		const Vector3 weights(0.0722f, 0.7152f, 0.2126f); // linear NTSC, R and B swapped (source: Wikipedia)
		const __m128 luma = _mm_dp_ps(weights, color, 0xff); // SSE 4.1
		return vLerp4(color, luma, amount);
	}
} 
