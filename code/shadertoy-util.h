
// cookiedough -- utilities to port Shadertoy to CPU

/*
	FIXME:
		- add sign() function
		- add UV function that supplies offset and delta instead (only makes sense if dropping OpenMP)
*/

#pragma once

#include "map-blitter.h"
using namespace FXMAP;

namespace Shadertoy
{
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

	const __m128 chanScale = _mm_set1_ps(255.f);

	// - writes 4 pixels at once
	// - assumes aligned input
	// - FIXME: swap R and B here?
	// - FIXME: shift instead of scale?
	VIZ_INLINE __m128i ToPixel4(const Vector4 *colors)
	{
		__m128i zero = _mm_setzero_si128();

		__m128i iA = _mm_max_epi32(zero, _mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[0].vSIMD)));
		__m128i iB = _mm_max_epi32(zero, _mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[1].vSIMD)));
		__m128i iC = _mm_max_epi32(zero, _mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[2].vSIMD)));
		__m128i iD = _mm_max_epi32(zero, _mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[3].vSIMD)));

//		__m128i iA = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[0].vSIMD)));
//		__m128i iB = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[1].vSIMD)));
//		__m128i iC = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[2].vSIMD)));
//		__m128i iD = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[3].vSIMD)));

		__m128i AB = _mm_packs_epi32(iA, iB);
		__m128i CD = _mm_packs_epi32(iC, iD);
		return (_mm_packus_epi16(AB, CD));
	}
} 
