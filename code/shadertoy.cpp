
// cookiedough -- simple Shadertoy ports

/*
	to do:
		- fix a *working* OpenMP implementation of the plasma (are it the split writes in 64-bit?)
		- a minor optimization is to get offsets and deltas to calculate current UV, but that won't parallelize with OpenMP
		- just optimize as needed, since all of this is a tad slow
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
	const float sine = 0.2f*lutsinf(point.x-point.y);
	const float fX = sine + lutcosf(point.x*0.33f);
	const float fY = sine + lutcosf(point.y*0.33f);
	const float fZ = sine + lutcosf(point.z*0.33f);
//	return sqrtf(fX*fX + fY*fY + fZ*fZ)-0.8f;
	return 1.f/Q_rsqrt(fX*fX + fY*fY + fZ*fZ)-0.8f;
}

static void RenderPlasmaMap(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 colMulA(0.1f, 0.15f, 0.3f);
	const Vector3 colMulB(0.05f, 0.05f, 0.1f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{

				auto& UV = Shadertoy::ToUV_FX_2x2(iX+iColor, iY, 2.f);

				const int cosIndex = tocosindex(time*0.314f);
				const float dirCos = lutcosf(cosIndex);
				const float dirSin = lutsinf(cosIndex);

				Vector3 direction(
					dirCos*UV.x - dirSin*0.6f,
					UV.y,
					dirSin*UV.x + dirCos*0.6f);

//				Vector3 direction(Shadertoy::ToUV_FX_2x2(iX+iColor, iY, 2.f), 0.6f);

				Vector3 origin = direction;
				for (int step = 0; step < 46; ++step)
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
// FIXME: radicate redundant operations
// FIXME: add fast noise function (see chat with Michiel)
// FIXME: improve look using more recent code (and look at chat for that Aura for Laura SDF)
//

VIZ_INLINE float fNautilus(const Vector3 &position, float time)
{
	float cosX, cosY, cosZ;
	cosX = lutcosf(lutcosf(position.x + time*0.125f)*position.x - lutcosf(position.y + time*0.11f)*position.y);
	cosY = lutcosf(position.z/3.f*position.x - lutcosf(time*0.142f)*position.y);
	cosZ = lutcosf(position.x + position.y + position.z/1.25f + time);
	const float total = cosX*cosX + cosY*cosY + cosZ*cosZ;
	return total-0.814f; // -1.f
}

// supposed "Aura for Laura" function
VIZ_INLINE float fLaura(const Vector3 &position)
{
	return cosf(position.x)+cosf(position.y*1.f)+cosf(position.z)+cosf(position.y*20.f)*0.5f;
}

static void RenderNautilusMap(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 colorization(
		.1f-cosf(time*0.33f)*0.05f,
		.1f, 
		.1314f+cosf(time*0.08f)*.0125f); 

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				// trick learned: larger grids create these Rez-style depths
//				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY,  1.f);
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, -3.14f);

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

				float nOffs = .012f;
				Vector3 normal(
					0.f, // march-fNautilus(Vector3(hit.x+nOffs, hit.y, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y+nOffs, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y, hit.z+nOffs), time));

//				Vector3 light(0.f, 0.5f, -.5f);
//				float origVerLight = normal*light; // std::max(0.f, normal*light);
				float origVerLight = normal.z*-0.5f + normal.y*0.5f + normal.z*0.5f;
				
				Vector3 color = colorization;
				color *= total/41.f;
//				color *= total*0.1f;
				color += origVerLight;

//				const float gamma = 1.88f;
//				colorization.x = powf(colorization.x, gamma);
//				colorization.y = powf(colorization.y, gamma);
//				colorization.z = powf(colorization.x, gamma);
				
				colors[iColor].vSIMD = color.vSIMD;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

static void RenderNautilusMap_4x4(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 colorization(
		.1f-cosf(time*0.33f)*0.05f,
		.1f, 
		.1314f+cosf(time*0.08f)*.0125f); 

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kCoarseResY; ++iY)
	{
		const int yIndex = iY*kCoarseResX;
		for (int iX = 0; iX < kCoarseResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_4x4(iColor+iX, iY, -3.14f);

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

				float nOffs = .015f;
				Vector3 normal(
					march-fNautilus(Vector3(hit.x+nOffs, hit.y, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y+nOffs, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y, hit.z+nOffs), time));

				float origVerLight = normal.z*-0.5f + normal.y*0.5f + normal.z*0.5f;
				
				Vector3 color = colorization;
				color *= total/41.f;
				color += origVerLight;

				// const float gamma = 1.88f;
				// color.x = powf(color.x, gamma);
				// color.y = powf(color.y, gamma);
				// color.z = powf(color.x, gamma);
				
				colors[iColor].vSIMD = color.vSIMD;
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

//	RenderNautilusMap_4x4(g_pFXCoarse, time);
//	MapBlitter_Colors_4x4(pDest, g_pFXCoarse);
}
