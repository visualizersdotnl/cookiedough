
// cookiedough -- simple marched plasma (idea: https://www.shadertoy.com/view/ldSfzm)

/*
	to do:
		- testbed for multi-threading & other optimizations (what makes it so slow?)
		- try to find a way to store and swap colors to SIMD earlier on
		- I might have to resort to interpolation anyway?
*/

#include "main.h"
// #include "plasma.h"
#include "image.h"
// #include "bilinear.h"
#include "shadertoy-util.h"

bool Plasma_Create()
{
	return true;
}

void Plasma_Destroy()
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
	return sqrtf(fX*fX + fY*fY + fZ*fZ)-0.8f;
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

		for (int iX = 0; iX < kResX; iX += 4)
		{	
			// FIXME
			Vector4 colors[4];

			for (int iColor = 0; iColor < 4; ++iColor)
			{
				const Vector3 direction(Shadertoy::ToUV(iX+iColor+20, iY+kHalfResY, 1.f), 0.5f);
				Vector3 origin = direction;

				for (int step = 0; step < 32; ++step)
					origin += direction*march(origin, time);

				const Vector3 color = ( colMulA*march(origin+direction, time)+colMulB*march(origin*0.5f, time) ) * (8.f - origin.x/2.f);
				colors[iColor].vSIMD = color.vSIMD;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Plasma_2_Draw(uint32_t *pDest, float time, float delta)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	#pragma omp parallel for
	for (int iY = 0; iY < kResY; ++iY)
	{
		const int yIndex = iY*kResX;
		for (int iX = 0; iX < kResX; iX += 4)
		{	
			// FIXME
			Vector4 colors[4];

			for (int iColor = 0; iColor < 4; ++iColor)
			{
				Vector2 UV = Shadertoy::ToUV(iX+iColor, iY+kResY, 4.f);
				UV += time*0.3f;

				float distortY = 0.1f + cosf(UV.y + sinf(0.148f - time)) + 2.4f*time;
				float distortX = 0.9f + sinf(UV.x + cosf(0.628f + time)) - 0.7f*time;
				float length = UV.Length();
				float total = 7.f*cosf(length+distortX)*sinf(distortY+distortX);

				Vector3 color(
					0.5f + 0.5f*cosf(total+0.2f),
					0.5f + 0.5f*cosf(total+0.5f),
					0.5f + 0.5f*cosf(total+0.9f));

				// FIXME: get a real slope value!
				float specZ = (color.z*color.z)/length;
				specZ = std::max(specZ, 0.f);
				specZ = powf(specZ, 4.f);
				color = color + Vector3(1.f, 0.7f, 0.4f)*specZ;

				colors[iColor] = Vector4(color, 0.f);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}
