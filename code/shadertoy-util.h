
// cookiedough -- utility functions to easily port Shadertoy

// missing:
// - sign()

#pragma once

namespace Shadertoy
{

	VIZ_INLINE const Vector2 ToUV(unsigned iX, unsigned iY)
	{
		float fX = (float) iX;
		float fY = (float) iY;
		fX *= 1.f/kResX;
		fY *= 1.f/kResY;
		return Vector2((fX-0.5f)*2.f, (fY-0.5f)*2.f);
	}

	VIZ_INLINE const Vector2 ToUV_RT(unsigned iX, unsigned iY)
	{
		float fX = (float) iX;
		float fY = (float) iY;
		fX *= 1.f/kTargetResX;
		fY *= 1.f/kTargetResY;
		return Vector2((fX-0.5f)*2.f, (fY-0.5f)*2.f);
	}

	VIZ_INLINE uint32_t ToPixel(Vector3 color)
	{
		// FIXME: use SSE!
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
}
