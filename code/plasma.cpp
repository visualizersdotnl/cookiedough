
// cookiedough -- simple marched plasma (idea: https://www.shadertoy.com/view/ldSfzm)

/*
	to do:
		- use as testbed for multi-threading & other optimizations
		- make leaner where possible
*/

#include "main.h"
// #include "plasma.h"
#include "image.h"
// #include "bilinear.h"
#include "shadertoy-util.h"

VIZ_INLINE float march(Vector3 point, float time)
{
	point.z += 5.f*time;
	float sine = 0.2f*cosf(point.x-point.y);
	return Vector3(sine+cosf(point.x/3.f), sine+cosf(point.y/3.f), sine+cosf(point.z/3.f)).Length() - 0.8f; // FIXME: remove divide
}

bool Plasma_Create()
{
	return true;
}

void Plasma_Destroy()
{
}

#include <omp.h>

void Plasma_Draw(uint32_t *pDest, float time, float delta)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 colMulA(0.3f, 0.15f, 0.1f);
	const Vector3 colMulB(0.1f, 0.05f, 0.f);

	#pragma omp parallel for
	for (int iY = 0; iY < kResY; ++iY)
	{
		for (int iX = 0; iX < kResX; iX += 4)
		{	
			// unroll?
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				const Vector3 direction(Shadertoy::ToUV(iX+iColor, iY+kHalfResY, 1.f), 0.5f);
				Vector3 origin = direction;

				for (int step = 0; step < 64; ++step)
					origin += direction*march(origin, time);

				const Vector3 color = ( colMulA*march(origin+direction, time)+colMulB*march(origin*0.5f, time) ) * (8.f - origin.x/2.f);

				colors[iColor] = Vector4(color.z, color.y, color.x, 0.f);
			}

			const int index = iX + iY*kResX;
			pDest128[index>>2] = Shadertoy::ToPixel4(colors);
		}
	}
}
