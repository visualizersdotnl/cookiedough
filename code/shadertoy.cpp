
// cookiedough -- simple Shadertoy ports & my own shaders

/*
	important:
		- *** R and B are swapped, as in: Vector3 color(B, R, G) ***
		- Vector3 and Vector4 are 16-bit aligned and have a vSIMD member you can use

	to do:
		- optimize where you can; ideally, in some situation, I'd handle more calculations in SIMD parallel
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
	return 1.f/Q3_rsqrtf(fX*fX + fY*fY + fZ*fZ)-0.8f;
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

// FIXME: maybe SIMD-ize this?
VIZ_INLINE float fNautilus(const Vector3 &position, float time)
{
	float cosX, cosY, cosZ;
	float cosTime7 = lutcosf(time*0.1428f); // ** i'm getting an internal compiler error if I write this where it should be **
	cosX = lutcosf(lutcosf(position.x + time*0.125f)*position.x - lutcosf(position.y + time/9.f)*position.y);
	cosY = lutcosf(position.z*0.33f*position.x - cosTime7*position.y);
	cosZ = lutcosf(position.x + position.y + position.z*0.8f + time);
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
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f;
				for (int iStep = 0; iStep < 64; ++iStep)
				{
					hit = origin + direction*total;
//					hit = direction*total;
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
				Shadertoy::vFastNorm3(normal);

				float diffuse = normal.z*0.1f;
				float specular = powf(std::max(0.f, normal*direction), 16.f);

				Vector3 color(diffuse);
				color += colorization*(1.56f*total + specular);
				color += specular*0.314f;

				colors[iColor].vSIMD = Shadertoy::GammaAdj(color, 2.22f);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Nautilus_Draw(uint32_t *pDest, float time, float delta)
{
	RenderNautilusMap_2x2(g_pFXFine, time);
	
	// FIXME: this looks amazing if timed!
	// HorizontalBoxBlur32(g_pFXFine, g_pFXFine, kFineResX, kFineResY, fabsf(0.1314f*sin(time)));
	
	MapBlitter_Colors_2x2(pDest, g_pFXFine);
}

//
// Aura for Laura cosine grid
//
// FIXME:
// - more animation, better colors?
// - animation: rig it to Rocket so that it can take a corner over any axis, and banking!
//

// cosine blob tunnel
VIZ_INLINE float fAuraForLaura(const Vector3 &position)
{	
	return lutcosf(position.x*1.314f)+lutcosf(position.y*0.314f)+lutcosf(position.z)+lutcosf(position.y*2.5f)*0.5f;
}

static void RenderLauraMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	Vector3 fogColor(0.8f, 0.9f, 0.1f);
	fogColor *= 0.4314f;

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
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f;
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
				Shadertoy::vFastNorm3(normal);

				float diffuse = normal.y*0.12f + normal.x*0.12f + normal.z*0.25f;
				float specular = powf(std::max(0.f, normal*direction), 24.f);
				float yMod = 0.5f + 0.5f*lutcosf(hit.y*48.f);

				Vector3 color(diffuse);
				color *= yMod;
				color += specular+specular;

				const float distance = hit.z-origin.z;
				Shadertoy::ApplyFog(distance, color.vSIMD, fogColor.vSIMD, 0.006f);

				colors[iColor].vSIMD = Shadertoy::GammaAdj(color, kGoldenRatio);
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

//
// Distorted sphere (spikes)
//
// FIXME:
// - animate spikes: switching the frequency per axis along with the rotation might look cool (mind the aliasing)
// - if breaking out of the march loop, skip lighting calculations and just output fog (doesn't really apply momentarily)
//

static Vector4 fTest_global;
VIZ_INLINE float fTest(const Vector3 &position) 
{
	constexpr float scale = kGoldenRatio*0.1f;
    const float radius = 1.35f + scale*lutcosf(fTest_global.y*position.y - fTest_global.x) + scale*lutcosf(fTest_global.z*position.x + fTest_global.x);
	return Shadertoy::vFastLen3(position) - radius; // return position.Length() - radius;
}

static void RenderSpikeyMap_2x2_Close(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	Vector3 fogColor(0.f);

	fTest_global = Vector4(2.5f*time, 16.f, 22.f, 0.f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, kGoldenRatio); // FIXME: possible parameter

				Vector3 origin(0.2f, 0.f, -2.23f); // FIXME: nice parameters too!
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rot2D(time*0.14f /* FIXME: phase parameter */, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f; 
				int iStep;
				for (iStep = 0; iStep < 33; ++iStep)
				{
					hit = origin + direction*total;
					march = fTest(hit);
					total += march*(0.1f*kGoldenRatio);

					// enable if not screen-filling, otherwise it just costs a lot of cycles
//					if (total < 0.01f || total > 10.f)
//					{
//						break;
//					}
				}

				float nOffs = 0.1f;
				Vector3 normal(
					march-fTest(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fTest(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fTest(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				float diffuse = normal.y*0.1f + normal.x*0.15f + normal.z*0.7f;
				const float specular = powf(std::max(0.f, normal*direction), 2.f);

				const float distance = hit.z-origin.z;

				Vector3 color(diffuse);
//				color += Shadertoy::CosPalSimplest(distance, 0.314f + 0.25f*diffuse, Vector3(0.2f, 0.1f, 0.6f), 0.5314f);
				color += Shadertoy::CosPalSimplest(distance, 0.314f + 0.25f*diffuse, Vector3(0.6f, 0.1f, 0.6f), 0.5314f);
				color += specular;

//				color.vSIMD = Shadertoy::Desaturate(color.vSIMD, 0.314f);
				Shadertoy::ApplyFog(distance, color.vSIMD, fogColor.vSIMD, 0.733f);

				colors[iColor].vSIMD = Shadertoy::GammaAdj(color, 1.99f);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

// FIXME: abandoned for a while, take hints from the one above
static void RenderSpikeyMap_2x2_Distant(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	Vector3 fogColor(0.09f, 0.01f, 0.08f);

	fTest_global = Vector4(2.5f*time, 16.f, 24.f, 0.f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			Vector4 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, kGoldenRatio); // FIXME: possible parameter

				Vector3 origin(0.f, 0.f, -3.314f);
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rot2D(time*0.0314f, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f; 
				int iStep;
				for (iStep = 0; iStep < 36; ++iStep)
				{
					hit = origin + direction*total;
					march = fTest(hit);
					total += march*(0.1f*kGoldenRatio);

					// enable if not screen-filling, otherwise it just costs a lot of cycles
//					if (total < 0.01f || total > 10.f)
//					{
//						break;
//					}
				}

				float nOffs = 0.1f;
				Vector3 normal(
					march-fTest(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fTest(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fTest(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				float diffuse = normal.y*0.1f + normal.x*0.25f + normal.z*0.5f;
				const float specular = powf(std::max(0.f, normal*direction), 3.14f);

				const float distance = origin.z-hit.z;

				Vector3 color = Shadertoy::CosPalSimple(distance, Vector3(0.6f, 0.f, fabsf(lutcosf(time*0.314f))), diffuse, Vector3(0.6f, 0.1f, 0.6f), .5f);
				color.Multiply(color);
				color += specular+specular;

				Shadertoy::ApplyFog(distance, color.vSIMD, fogColor.vSIMD, 0.314f);

				colors[iColor].vSIMD = Shadertoy::GammaAdj(color, kGoldenRatio);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Spikey_Draw(uint32_t *pDest, float time, float delta, bool close /* = true */)
{
	if (close)
		RenderSpikeyMap_2x2_Close(g_pFXFine, time);
	else
		RenderSpikeyMap_2x2_Distant(g_pFXFine, time);

	MapBlitter_Colors_2x2(pDest, g_pFXFine);
}
