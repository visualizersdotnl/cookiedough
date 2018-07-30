
// cookiedough -- simple Shadertoy ports

/*
	to do:
		- fix a *working* OpenMP implementation of the plasma (are it the split writes in 64-bit?)
		- a minor optimization is to get offsets and deltas to calculate current UV, but that won't parallelize with OpenMP
		- just optimize!
*/

#include "main.h"
// #include "shadertoy.h"
#include "image.h"
// #include "bilinear.h"
#include "shadertoy-util.h"
#include "boxblur.h"

bool Shadertoy_Create()
{
	return true;
}

void Shadertoy_Destroy()
{
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

static void RenderPlasmaMap(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 colMulA(0.11f, 0.15f, 0.29f);
	const Vector3 colMulB(0.01f, 0.05f, 0.12f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				const Vector3 direction(Shadertoy::ToUV_FX_2x2(iX+iColor+30, iY+166, 0.5f+0.214f), 0.614f);

				Vector3 origin = direction;
				for (int step = 0; step < 42; ++step)
					origin += direction*fPlasma(origin, time);
				
				colors[iColor] = colMulA*fPlasma(origin+direction, time) + colMulB*fPlasma(origin*0.5f, time); 
				colors[iColor] *= 8.f - origin.x*0.5f;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Plasma_Draw(uint32_t *pDest, float time, float delta)
{
	RenderPlasmaMap(g_pFXFine, time);
	MapBlitter_Colors_2x2(pDest, g_pFXFine);
//	HorizontalBoxBlur32(pDest, pDest, kResX, kResY, 0.01f);
}

//
// Nautilus by Michiel v/d Berg (https://www.shadertoy.com/view/MdXGz4)
//
// FIXME: use cosine LUT & eradicate redundant operations
// FIXME: add fast noise function (see chat with Michiel)
// FIXME: improve look using more recent code (and look at chat for that Aura for Laura SDF)
// FIXME: more shader ports!
//

VIZ_INLINE float fNautilus(const Vector3 &position, float time)
{
	float cosX = cosf(cosf(position.x + time/8.f)*position.x - cosf(position.y + time/9.f)*position.y);
	float cosY = cosf(position.z/3.f*position.x - cosf(time/7.f)*position.y);
	float cosZ = cosf(position.x + position.y + position.z/1.25f + time);
	float total = cosX*cosX + cosY*cosY + cosZ*cosZ;
	return total-1.f;
}

// supposed "Aura for Laura" function
VIZ_INLINE float fLaura(const Vector3 &position)
{
	return cosf(position.x)+cosf(position.y*1.f)+cosf(position.z)+cosf(position.y*20.f)*0.5f;
}

static void RenderNautilusMap(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, 2.f);

				Vector3 origin(0.f);
				Vector3 direction(UV.x, UV.y, 1.f);
				direction *= 1.f/64.f;

				Vector3 hit(0.f);

				float march = 1.f;
				float total = 0.f;
				for (int iStep = 0; iStep < 64*2; ++iStep)
				{
					if (march > .4f) // FIXE: slow!
					{
						total = (1.f+iStep)*2.f;
						hit = origin + direction*total;
						march = fNautilus(hit, time);
					}
				}

				float nOffs = .1f;
				Vector3 normal(
					march-fNautilus(Vector3(hit.x+nOffs, hit.y, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y+nOffs, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y, hit.z+nOffs), time));

				Vector3 light(0.f, 0.5f, 0.6f);
				float origVerLight = normal*light; // std::max(0.f, normal*light);

				Vector3 colorization(
					.1f-cosf(time/3.f)/19.f,
					.1f, 
					.1f+cosf(time/14.f)/8.f); 
				
				colorization *= total/41.f;
				colorization += origVerLight;

				// const float gamma = 2.20f;
				// colorization.x = powf(colorization.x, gamma);
				// colorization.y = powf(colorization.y, gamma);
				// colorization.z = powf(colorization.x, gamma);
				
				colors[iColor].vSIMD = colorization.vSIMD;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Nautilus_Draw(uint32_t *pDest, float time, float delta)
{
	RenderNautilusMap(g_pFXFine, time);
	MapBlitter_Colors_2x2(pDest, g_pFXFine);
}
