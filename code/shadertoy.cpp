
// cookiedough -- simple marched plasma (idea: https://www.shadertoy.com/view/ldSfzm)

/*
	to do:
		- testbed for multi-threading & other optimizations (what makes it so slow?)
		- I might have to resort to interpolation anyway!
		- create functions that allow me to loop over the UVs using increments instead of calculating it per pixel
*/

#include "main.h"
// #include "shadertoy.h"
#include "image.h"
// #include "bilinear.h"
#include "shadertoy-util.h"

bool Shadertoy_Create()
{
	return true;
}

void Shadertoy_Destroy()
{
}

VIZ_INLINE float march(Vector3 point, float time)
{
	point.z += 5.f*time;
	const float sine = 0.314f*lutsinf(ftofp24(point.x-point.y));
	const int x = ftofp24(point.x*0.33f);
	const int y = ftofp24(point.y*0.33f);
	const int z = ftofp24(point.z*0.33f);
	const float fX = sine + lutcosf(x);
	const float fY = sine + lutcosf(y);
	const float fZ = sine + lutcosf(z);
//	return sqrtf(fX*fX + fY*fY + fZ*fZ)-0.8f;
	return 1.f/Q_rsqrt(fX*fX + fY*fY + fZ*fZ)-0.8f;
}

void Plasma_1_Draw(uint32_t *pDest, float time, float delta)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

//	const Vector3 colMulA(0.3f, 0.15f, 0.1f);
//	const Vector3 colMulB(0.1f, 0.05f, 0.f);

	// swapped R and B
	const Vector3 colMulA(0.1f, 0.15f, 0.3f);
	const Vector3 colMulB(0.0f, 0.05f, 0.1f);

	#pragma omp parallel for
	for (int iY = 0; iY < kResY; ++iY)
	{
		const int yIndex = iY*kResX;

		Vector4 colors[4];
		for (int iX = 0; iX < kResX; iX += 4)
		{	
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				const Vector3 direction(Shadertoy::ToUV(iX+iColor+30, iY+166, 0.5f+0.214f), 0.614f);

				Vector3 origin = direction;
				for (int step = 0; step < 42; ++step)
					origin += direction*march(origin, time);

				colors[iColor] = colMulA*march(origin+direction, time);
				colors[iColor] += colMulB*march(origin*0.5f, time); 
				colors[iColor] *= 8.f - origin.x*0.5f;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}
