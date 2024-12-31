
// cookiedough -- voxel shared (FIXME: most of this (math) stuff can be replaced by Std3DMath)

#pragma once

namespace voxel
{
	VIZ_INLINE void vrot2D(float cosine, float sine, float &X, float &Y)
	{
		const float rotX = cosine*X - sine*Y;
		const float rotY = sine*X + cosine*Y;
		X = rotX;
		Y = rotY;
	}

	VIZ_INLINE void vnorm2D(float &X, float &Y)
	{
		if (X+Y != 0.f)
		{
			const float length = 1.f/sqrtf(X*X + Y*Y);
			X *= length;
			Y *= length;
		}
	}

	VIZ_INLINE void calc_fandeltas(float curAngle, float &dX, float &dY)
	{
		// FIXME: these should be more precise (and the inverse a reactant of that possibly), am I feeding bogus values
		//        in the case where I needed to singlethread the initial pass in ball.cpp?
		dX = cosf(curAngle); 
		dY = sinf(curAngle);
		return vnorm2D(dX, dY);
	}
}
