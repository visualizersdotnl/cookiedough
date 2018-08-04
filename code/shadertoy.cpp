
// cookiedough -- simple Shadertoy ports & my own shaders

/*
	important:
		- *** R and B are swapped, as in: Vector3 color(B, R, G) ***
		- Vector3 and Vector4 are 16-bit aligned and can cast to __m128 (SIMD) once needed
		- the "to UV" functions in shader-util.h take aspect ratio into account, you don't always want this (see spikey objects for example)
		- when scaling a vector by a scalar in a loop, write it instead of using the operator (which won't inline for one or another reason)
		- if needed, parts of loops can be parallelized (SIMD), but that's a lot of hassle
		- another obvious optimization is to get offsets and deltas to calculate current UV, but that won't parallelize with OpenMP

	to do:
		- optimize where you can (which does not always mean more SIMD, but it *does* for color operations)
		- what to do with hardcoded colors, paremeters?
*/

#include "main.h"
// #include "shadertoy.h"
#include "image.h"
// #include "bilinear.h"
#include "shadertoy-util.h"
#include "boxblur.h"
#include "rocket.h"

// Aura for Laura sync.:
SyncTrack trackLauraZ, trackLauraRoll;

bool Shadertoy_Create()
{
	trackLauraZ = Rocket::AddTrack("lauraZ");
	trackLauraRoll = Rocket::AddTrack("lauraRoll");

	return true;
}

void Shadertoy_Destroy()
{
}

//
// Plasma (https://www.shadertoy.com/view/ldSfzm)
//

VIZ_INLINE float fPlasma(const Vector3 &point, float time)
{
	const float sine = 0.2f*lutsinf(point.x-point.y);
	const float fX = sine + lutcosf(point.x*0.33f);
	const float fY = sine + lutcosf(point.y*0.33f);
	const float fZ = sine + lutcosf((5.f*time+point.z)*0.33f);
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
			__m128 colors[4];
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
				for (int step = 0; step < 34; ++step)
				{
					float march = fPlasma(origin, time);
					if (fabsf(march) < 0.001f)
						break;

//					origin += direction*march;
					origin.x += direction.x*march;
					origin.y += direction.y*march;
					origin.z += direction.z*march;
				}
				
				Vector3 color = colMulA*fPlasma(origin+direction, time) + colMulB*fPlasma(origin*0.5f, time); 
				color *= 8.f - origin.x*0.5f;

				colors[iColor] = color;
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

static Vector3 fNautilus_global;
VIZ_INLINE float fNautilus(const Vector3 &position, float time)
{
	const float cosX = lutcosf(lutcosf(position.x + fNautilus_global.x)*position.x - lutcosf(position.y + fNautilus_global.y)*position.y);
	const float cosY = lutcosf(position.z*0.33f*position.x - fNautilus_global.z*position.y);
	const float cosZ = lutcosf(position.x + position.y + position.z*0.8f + time);
	const float dotted = cosX*cosX + cosY*cosY + cosZ*cosZ;
	return dotted*0.5f - .7f;
};

static void RenderNautilusMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	fNautilus_global.x = time*0.125f;
	fNautilus_global.y = time/9.f;
	fNautilus_global.z = lutcosf(time*0.1428f);

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
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, 2.f); // FIXME: possible parameter

//				Vector3 origin(0.f);
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rot2D(kPI*lutcosf(time*0.06234f), direction.y, direction.x);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float total = 0.f;
				float march;
				for (int iStep = 0; iStep < 64; ++iStep)
				{
					hit = direction*total;
//					hit = origin + direction*total;
//					hit.x = direction.x*total;
//					hit.y = direction.y*total;
//					hit.z = direction.z*total;
					march = fNautilus(hit, time);
					total += march*0.628f;
					
//					if (fabsf(march) < 0.001f*(total*0.125f + 1.f) || total>20.f)
//						break;

					// cheaper, good gain
					if (fabsf(march) < 0.001f)
						break;
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

				colors[iColor] = Shadertoy::GammaAdj(color, 2.22f);
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

	float lauraZ = Rocket::getf(trackLauraZ);
	float lauraRoll = Rocket::getf(trackLauraRoll);

	Vector3 fogColor(0.8f, 0.9f, 0.1f);
	fogColor *= 0.4314f;

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY; ++iY)
	{
		const int yIndex = iY*kFineResX;
		for (int iX = 0; iX < kFineResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, 2.f); // FIXME: possible parameter

				Vector3 origin(0.f, 0.f, lauraZ);
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rot2D(lauraRoll, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f;
				int iStep;
				for (iStep = 0; iStep < 48; ++iStep)
				{
					// hit = origin + direction*total;
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;
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
				__m128 fogged = Shadertoy::ApplyFog(distance, color, fogColor, 0.006f);

				colors[iColor] = Shadertoy::GammaAdj(fogged, kGoldenRatio);
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
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, kGoldenRatio); // FIXME: possible parameter

				Vector3 origin(0.2f, 0.f, -2.23f); // FIXME: nice parameters too!
				Vector3 direction(UV.x/kAspect, UV.y, 1.f); 
				Shadertoy::rot2D(time*0.14f /* FIXME: phase parameter */, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f; 
				int iStep;
				for (iStep = 0; iStep < 33; ++iStep)
				{
					// hit = origin + direction*total;
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;
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
				color += Shadertoy::CosPalSimplest(distance, 0.314f + 0.25f*diffuse, Vector3(0.2f, 0.1f, 0.6f), 0.5314f);
//				color += Shadertoy::CosPalSimplest(distance, 0.314f + 0.25f*diffuse, Vector3(0.6f, 0.1f, 0.6f), 0.5314f);
				color += specular;

//				__m128 desaturated = Shadertoy::Desaturate(color, 0.314f);
				__m128 fogged = Shadertoy::ApplyFog(distance, color, fogColor, 0.733f);

				colors[iColor] = Shadertoy::GammaAdj(fogged, 1.99f);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

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
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FX_2x2(iColor+iX, iY, kGoldenRatio); // FIXME: possible parameter

				Vector3 origin(0.f, 0.f, -3.314f);
				Vector3 direction(UV.x/kAspect, UV.y, 1.f); 
				Shadertoy::rot2D(time*0.0314f, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f; 
				int iStep;
				for (iStep = 0; iStep < 36; ++iStep)
				{
					// hit = origin + direction*total;
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;
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

				__m128 fogged = Shadertoy::ApplyFog(distance, color, fogColor, 0.314f);

				colors[iColor] = Shadertoy::GammaAdj(fogged, kGoldenRatio);
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

	// HorizontalBoxBlur32(g_pFXFine, g_pFXFine, kFineResX, kFineResY, 0.1f);

	MapBlitter_Colors_2x2_interlaced(pDest, g_pFXFine);
}
