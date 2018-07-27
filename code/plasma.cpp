
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
	float sine = 0.2f*sinf(point.x-point.y);
	return Vector3(sine+cosf(point.x*0.33f), sine+cosf(point.y*0.33f), sine+cosf(point.z*0.33f)).Length() - 0.8f; // FIXME: remove divide
}

bool Plasma_Create()
{
	return true;
}

void Plasma_Destroy()
{
}

void Plasma_Draw(uint32_t *pDest, float time, float delta)
{
	const Vector3 colMulA(0.3f, 0.15f, 0.1f);
	const Vector3 colMulB(0.1f, 0.05f, 0.f);

	for (float fY = 0; fY < kResY; ++fY)
	{
		for (unsigned fX = 0; fX < kResX; ++fX)
		{	
			Vector3 direction((0.5f-fX)/kResX, (0.5f-fY)/kResY, 0.5f);
			Vector3 origin = direction;

			for (unsigned step = 0; step < 64; ++step)
				origin += direction*march(origin, time);

			Vector3 color = /* FIXME: abs() */ ( colMulA*march(origin+direction, time)+colMulB*march(origin*0.5f, time) ) * (8.f - origin.x/2.f);
	
			*pDest++ = Shadertoy::ToPixel(color);
		}
	}
}
