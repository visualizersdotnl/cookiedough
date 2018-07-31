
// cookiedough -- simple Shadertoy ports & my own shaders

/*
	important:
		- R and B are swapped, as in: Vector3 color(B, R, G)
		- Vector3 and Vector4 are 16-bit aligned and have a vSIMD member may you want to parallelize locally

	to do:
		- just optimize as needed, since all of this is a tad slow
		- what to do with hardcoded colors, paremeters?
		- a minor optimization is to get offsets and deltas to calculate current UV, but that won't parallelize with OpenMP!
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
				// FIXME: parametrize (minus gives a black bar on the left, ideal for an old school logo)
//				auto& UV = Shadertoy::ToUV_FX_2x2(iX+iColor-50, iY+20, 2.f);
				auto& UV = Shadertoy::ToUV_FX_2x2(iX+iColor+10, iY+20, 2.f);

				const int cosIndex = tocosindex(time*0.314f*0.5f);
				const float dirCos = lutcosf(cosIndex);
				const float dirSin = lutsinf(cosIndex);

				Vector3 direction(
					dirCos*UV.x - dirSin*0.75f,
					UV.y,
					dirSin*UV.x + dirCos*0.75f);

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
// Nautilus Redux by Michiel v/d Berg
//

VIZ_INLINE float fNautilus(const Vector3 &position, float time)
{
	float cosX, cosY, cosZ;
	float cosTime7 = lutcosf(time/7.f); // ** i'm getting an internal compiler error if I write this where it should be **
	cosX = lutcosf(lutcosf(position.x + time*0.125f)*position.x - lutcosf(position.y + time/9.f)*position.y);
	cosY = lutcosf(position.z*0.33f*position.x - cosTime7*position.y);
	cosZ = lutcosf(position.x + position.y + position.z/1.25f + time);
	const float dotted = cosX*cosX + cosY*cosY + cosZ*cosZ;
	return dotted*0.5f - .7f;
};

static void RenderNautilusMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	// FIXME: parametrize (or at least a blend between sets)
	const Vector3 colorization(
		.1f-lutcosf(time/3.f)/19.f, 
		.1f, 
		.1f+lutcosf(time/14.f)/8.f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, 2.f); // FIXME: possible parameter

				Vector3 origin(0.f);
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rot2D(kPI*lutcosf(time*0.06234f), direction.y, direction.x);
				direction *= 1.f/direction.Length();

				Vector3 hit;

				float march, total = 0.f;
				for (int iStep = 0; iStep < 64; ++iStep)
				{
					hit = origin + direction*total;
					march = fNautilus(hit, time);
					total += march*(0.314f+0.314f);

//					if (fabsf(march) < 0.001f*(total*0.125f + 1.f) || total>20.f)
//						break;
				}

				float nOffs = 0.1f;
				Vector3 normal(
					march-fNautilus(Vector3(hit.x+nOffs, hit.y, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y+nOffs, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y, hit.z+nOffs), time));
				normal *= 1.f/normal.Length();

//				float diffuse = normal.z*1.f;
				float diffuse = normal.z*0.1f;
				float specular = powf(std::max(0.f, normal*direction), 16.f);

				Vector3 color(diffuse);
				color += colorization*(1.56f*total + specular);
				color += specular*0.314f;

				const float gamma = 2.22f;
				color.x = powf(color.x, gamma);
				color.y = powf(color.y, gamma);
				color.z = powf(color.z, gamma);

				colors[iColor].vSIMD = color.vSIMD;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

static void RenderNautilusMap_4x4(uint32_t *pDest, float time)
{
	// FIXME: implement
	VIZ_ASSERT(false);
}

void Nautilus_Draw(uint32_t *pDest, float time, float delta)
{
	RenderNautilusMap_2x2(g_pFXFine, time);
	
	// FIXME: this looks amazing if timed!
	// HorizontalBoxBlur32(g_pFXFine, g_pFXFine, kFineResX, kFineResY, fabsf(0.1314f*sin(time)));
	
	MapBlitter_Colors_2x2(pDest, g_pFXFine);

//	RenderNautilusMap_4x4(g_pFXCoarse, time);
//	MapBlitter_Colors_4x4(pDest, g_pFXCoarse);
}

//
// Aura for Laura cosine grid.
//
// FIXME:
// - proportions, colors, fog
// - animation: rig it to Rocket so that it can take a corner over any axis, and banking!
//

// cosine blob tunnel
VIZ_INLINE float fAuraForLaura(const Vector3 &position)
{
	return lutcosf(position.x*1.f)+lutcosf(position.y*0.314f)+lutcosf(position.z)+lutcosf(position.y*2.5f)*0.5f;
}

static void RenderLauraMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	Vector3 fogColor(0.3f, 0.2f, 0.1f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, 2.f); // FIXME: possible parameter

				Vector3 origin(0.f, 0.f, time*8.f);
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rot2D(time*0.05234f, direction.x, direction.y);
				direction *= 1.f/direction.Length();

				Vector3 hit;

				float march, total = 0.5f;
				int iStep;
				for (iStep = 0; iStep < 48; ++iStep)
				{
					hit = origin + direction*total;
					march = fAuraForLaura(hit);
					total += march*0.7314f;
				}

				float nOffs = 0.1f;
				Vector3 normal(
					march-fAuraForLaura(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fAuraForLaura(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fAuraForLaura(Vector3(hit.x, hit.y, hit.z+nOffs)));
				normal *= 1.f/normal.Length();

				float diffuse = normal.y*0.12f + normal.x*0.12f + normal.z*0.25f;
				float specular = powf(std::max(0.f, normal*direction), 16.f);
				float yMod = 0.5f + 0.5f*lutcosf(hit.y*48.f);

				Vector3 color(diffuse);
				color += specular;
				color *= yMod;

				const float distance = origin.z-hit.z;
				const float fog = 1.f-(expf(-0.006f*distance*distance));
				color = lerpf(color, fogColor, fog);

				const float gamma = 1.42f;
				color.x = powf(color.x, gamma);
				color.y = powf(color.y, gamma);
				color.z = powf(color.z, gamma);

				colors[iColor].vSIMD = color.vSIMD;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Laura_Draw(uint32_t *pDest, float time, float delta)
{
	RenderLauraMap_2x2(g_pFXFine, time);
	MapBlitter_Colors_2x2(pDest, g_pFXFine);
}
