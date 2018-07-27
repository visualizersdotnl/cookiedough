
// cookiedough -- utility functions to easily port Shadertoy

#pragma once

VIZ_INLINE const Vector2 Shadertoy_ToCoord(unsigned iX, unsigned iY)
{
	float fX = (float) iX;
	float fY = (float) iY;
	fX *= 1.f/kResX;
	fY *= 1.f/kResY;
	return Vector2((fX-0.5f)*2.f, (fY-0.5f)*2.f);
}

VIZ_INLINE const Vector2 Shadertoy_ToCoord_RT(unsigned iX, unsigned iY)
{
	float fX = (float) iX;
	float fY = (float) iY;
	fX *= 1.f/kTargetResX;
	fY *= 1.f/kTargetResY;
	return Vector2((fX-0.5f)*2.f, (fY-0.5f)*2.f);
}

/* IMPL MAKE FAST
inline float sign(float x)
{
	if (x>0.f) return 1.f;
	if (x<0.f) return -1.f;
	else return 0.f;
}
*/