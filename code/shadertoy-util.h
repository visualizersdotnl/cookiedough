
// cookiedough -- utilities to port Shadertoy to CPU

/*
	FIXME:
		- add sign() function
*/

#pragma once

// #include <omp.h>
#pragma warning(disable:4199)

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

	/*
	VIZ_INLINE uint32_t ToPixel_Ref(Vector3 color)
	{
		color.x = fabsf(color.x);
		color.y = fabsf(color.y);
		color.z = fabsf(color.z);
		if (color.x > 1.f) color.x = 1.f;
		if (color.y > 1.f) color.y = 1.f;
		if (color.z > 1.f) color.z = 1.f;
		int R = int(color.x*255.f);
		int G = int(color.y*255.f);
		int B = int(color.z*255.f);
		return B | G<<8 | R<<16;
	}
	*/

	const __m128 chanScale = _mm_set1_ps(255.f);

	// - writes 4 pixels at once (their clamped absolute value)
	// - assumes aligned input
	// - FIXME: swap R and B here? / shift, not scale
	VIZ_INLINE __m128i ToPixel4(const Vector4 *colors)
	{
		__m128i iA = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[0].vSIMD)));
		__m128i iB = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[1].vSIMD)));
		__m128i iC = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[2].vSIMD)));
		__m128i iD = _mm_abs_epi32(_mm_cvtps_epi32(_mm_mul_ps(chanScale, colors[3].vSIMD)));
		__m128i AB = _mm_packs_epi32(iA, iB);
		__m128i CD = _mm_packs_epi32(iC, iD);
		return (_mm_packus_epi16(AB, CD));
	}
} 
