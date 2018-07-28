
// cookiedough -- simple Shadertoy ports

/*
	to do:
		- fix a *working* OpenMP implementation of the plasma
		- create functions that allow me to loop over the UVs using increments instead of calculating it per pixel
*/

#include "main.h"
// #include "shadertoy.h"
#include "image.h"
// #include "bilinear.h"
#include "shadertoy-util.h"

static uint32_t *s_pFXMap = nullptr;

bool Shadertoy_Create()
{
	s_pFXMap = static_cast<uint32_t*>(mallocAligned(kFXMapBytes, kCacheLine));

	return true;
}

void Shadertoy_Destroy()
{
	freeAligned(s_pFXMap);
}

//
// Plasma (https://www.shadertoy.com/view/ldSfzm)
//

VIZ_INLINE float fPlasma(Vector3 point, float time)
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

void RenderPlasmaMap(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 colMulA(0.11f, 0.15f, 0.29f);
	const Vector3 colMulB(0.01f, 0.05f, 0.12f);

	for (int iY = 0; iY < kFXMapResY; ++iY)
	{
		const int yIndex = iY*kFXMapResX;
		for (int iX = 0; iX < kFXMapResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				const Vector3 direction(Shadertoy::ToUV_FX(iX+iColor+30, iY+166, 0.5f+0.214f), 0.614f);

				Vector3 origin = direction;
				for (int step = 0; step < 42; ++step)
					origin += direction*fPlasma(origin, time);

				colors[iColor] = colMulA*fPlasma(origin+direction, time);
				colors[iColor] += colMulB*fPlasma(origin*0.5f, time); 
				colors[iColor] *= 8.f - origin.x*0.5f;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Plasma_Draw(uint32_t *pDest, float time, float delta)
{
	RenderPlasmaMap(s_pFXMap, time);
	MapBlitter_Colors(pDest, s_pFXMap);
}
