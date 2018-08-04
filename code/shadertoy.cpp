
// cookiedough -- simple Shadertoy ports & my own shaders

/*
	important:
		- *** R and B are swapped, as in: Vector3 color(B, R, G) ***
		- Vector3 and Vector4 are 16-bit aligned and can cast to __m128 (SIMD) once needed
		- the "to UV" functions in shader-util.h take aspect ratio into account, you don't always want this (see spikey objects for example)
		- when scaling a vector by a scalar in a loop, write it in place instead of using the operator (which won't inline for one or another reason)
		- if needed, parts of loops can be parallelized (SIMD), but that's a lot of hassle
		- another obvious optimization is to get offsets and deltas to calculate current UV, but that won't parallelize with OpenMP

	to do:
		- optimize where you can (which does not always mean more SIMD, but it *does* for color operations)
		- what to do with hardcoded colors, paremeters?
*/

#include "main.h"
// #include "shadertoy.h"
#include "image.h"
#include "bilinear.h"
#include "shadertoy-util.h"
#include "boxblur.h"
#include "rocket.h"

// --- Sync. tracks ---

// Aura for Laura sync.:
SyncTrack trackLauraSpeed;
SyncTrack trackLauraYaw, trackLauraPitch, trackLauraRoll;

// Nautilus sync.:
SyncTrack trackNautilusRoll;

// Spike (close) sync.:
SyncTrack trackSpikeSpeed;
SyncTrack trackSpikeRoll;
SyncTrack trackSpikeSpecular;
SyncTrack trackDistSpikeWarmup; // specular-only glow blur effect, yanked from Aura for Laura

// --------------------

static uint32_t *s_pFDTunnelTex = nullptr;

bool Shadertoy_Create()
{
	// Aura for Laura:
	trackLauraSpeed = Rocket::AddTrack("lauraSpeed");
	trackLauraYaw = Rocket::AddTrack("lauraYaw");
	trackLauraPitch = Rocket::AddTrack("lauraPitch");
	trackLauraRoll = Rocket::AddTrack("lauraRoll");

	// Nautilus:
	trackNautilusRoll = Rocket::AddTrack("nautilusRoll");

	// Spikes:
	trackSpikeSpeed = Rocket::AddTrack("cSpikeSpeed");
	trackSpikeRoll = Rocket::AddTrack("cSpikeRoll");
	trackSpikeSpecular = Rocket::AddTrack("cSpikeSpecular");
	trackDistSpikeWarmup = Rocket::AddTrack("cDistSpikeWarmup");

	s_pFDTunnelTex = Image_Load32("assets/shadertoy/grid2b.jpg");
	if (nullptr == s_pFDTunnelTex)
		return false;

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
	const float fY = sine + lutcosf(point.y*0.43f);
	const float fZ = sine + lutcosf((5.f*time+point.z)*0.53f);
//	return sqrtf(fX*fX + fY*fY + fZ*fZ)-0.8f;
	return 1.f/Q3_rsqrtf(fX*fX + fY*fY + fZ*fZ)-0.8f;
}

static void RenderPlasmaMap(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 colMulA(0.1f, 0.15f, 0.3f);
	const Vector3 colMulB(0.05f, 0.05f, 0.1f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				// FIXME: parametrize (minus gives a black bar on the left, ideal for an old school logo)
//				auto& UV = Shadertoy::ToUV_FxMap(iX+iColor-50, iY+20, 2.f);
				auto& UV = Shadertoy::ToUV_FxMap(iX+iColor+10, iY+20, 2.f);

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
	RenderPlasmaMap(g_pFxMap, time);
	Fx_Blit_2x2(pDest, g_pFxMap);
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

	float roll = Rocket::getf(trackNautilusRoll);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 2.f); // FIXME: possible parameter

//				Vector3 origin(0.f);
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float total = 0.01f;
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
	RenderNautilusMap_2x2(g_pFxMap, time);
	Fx_Blit_2x2(pDest, g_pFxMap);
	
	// FIXME: this looks amazing if timed!
	// HorizontalBoxBlur32(g_pFxMap, g_pFxMap, kFxMapResX, kFxMapResY, fabsf(0.1314f*sin(time)));
	
}

//
// Aura for Laura cosine grid
//
// FIXME:
// - more animation, better colors?
//

// cosine blob tunnel
VIZ_INLINE float fAuraForLaura(const Vector3 &position)
{	
	const float grid = lutcosf(position.x*1.14f)+lutcosf(position.y*0.314f)+lutcosf(position.z)+lutcosf(position.y*kGoldenRatio)*0.628f;
	return grid;
}

VIZ_INLINE const Vector2 fLauraPath(float Z)
{
	return { lutcosf(Z*0.33f)*0.314f, lutsinf(-Z*0.44f)*0.314f };
}

static void RenderLauraMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const float lauraZ = time*Rocket::getf(trackLauraSpeed);
	const float lauraYaw = Rocket::getf(trackLauraYaw);
	const float lauraPitch = Rocket::getf(trackLauraPitch);
	const float lauraRoll = Rocket::getf(trackLauraRoll);

	Vector3 fogColor(0.8f, 0.9f, 0.1f);
	fogColor *= 0.4314f;

	const Vector3 origin(fLauraPath(lauraZ), lauraZ);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 2.f); // FIXME: possible parameter

				Vector3 direction(UV.x, UV.y, 1.f); 
				
				// FIXME: use 3x3 matrix?
				Shadertoy::rotY(lauraYaw,   direction.x, direction.z);
				Shadertoy::rotX(lauraPitch, direction.y, direction.z);
				Shadertoy::rotZ(lauraRoll,  direction.x, direction.y);
				
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.1f;
				for (int iStep = 0; iStep < 48; ++iStep)
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

				float diffuse = normal.y*0.12f + normal.x*0.12f + normal.z*0.35f;
				float specular = powf(std::max(0.f, normal*direction), 16.f);
				float yMod = 0.5f + 0.5f*lutcosf(hit.y*48.f);

				Vector3 color(diffuse);
				color *= yMod;
				color += specular+specular;

				const float distance = hit.z-origin.z;
				__m128 fogged = Shadertoy::ApplyFog(distance, color, fogColor, 0.006f);

				colors[iColor] = Shadertoy::GammaAdj(fogged, 1.614f);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Laura_Draw(uint32_t *pDest, float time, float delta)
{
	RenderLauraMap_2x2(g_pFxMap, time);
	Fx_Blit_2x2(pDest, g_pFxMap);
}

//
// Distorted sphere (spikes)
//
// FIXME:
// - animate spikes: switching the frequency per axis along with the rotation might look cool (mind the aliasing)
// - if breaking out of the march loop, skip lighting calculations and just output fog (doesn't really apply momentarily)
//

static Vector4 fTest_global;
VIZ_INLINE float fSpikey1(const Vector3 &position) 
{
	constexpr float scale = kGoldenRatio*0.1f;
    const float radius = 1.35f + scale*lutcosf(fTest_global.y*position.y - fTest_global.x) + scale*lutcosf(fTest_global.z*position.x + fTest_global.x);
	return Shadertoy::vFastLen3(position) - radius; // return position.Length() - radius;
}

static void RenderSpikeyMap_2x2_Close(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	Vector3 fogColor(0.f);

	const float speed = Rocket::getf(trackSpikeSpeed);
	const float roll = Rocket::getf(trackSpikeRoll);
	const float specPow = Rocket::getf(trackSpikeSpecular);

	fTest_global = Vector4(speed*time, 16.f, 22.f, 0.f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, kGoldenRatio); // FIXME: possible parameter

				Vector3 origin(0.2f, 0.f, -2.23f); // FIXME: nice parameters too!
				Vector3 direction(UV.x/kAspect, UV.y, 1.f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.01f; 
				int iStep;
				for (iStep = 0; iStep < 33; ++iStep)
				{
					// hit = origin + direction*total;
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;
					march = fSpikey1(hit);
					total += march*(0.1f*kGoldenRatio);

					// enable if not screen-filling, otherwise it just costs a lot of cycles
//					if (total < 0.01f || total > 10.f)
//					{
//						break;
//					}
				}

				float nOffs = 0.1f;
				Vector3 normal(
					march-fSpikey1(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSpikey1(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSpikey1(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				float diffuse = normal.y*0.1f + normal.x*0.15f + normal.z*0.7f;
				const float specular = powf(std::max(0.f, normal*direction), specPow);

				const float distance = hit.z-origin.z;

				Vector3 color(diffuse);
				color += Shadertoy::CosPalSimplest(distance, 0.314f + 0.25f*diffuse, Vector3(0.2f, 0.1f, 0.6f), 0.5314f);
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

VIZ_INLINE float fSpikey2(const Vector3 &position) 
{
	constexpr float scale = kGoldenRatio*0.1f;
    const float radius = 1.35f + scale*lutcosf(fTest_global.y*position.y - fTest_global.x) + scale*lutcosf(fTest_global.z*position.x + fTest_global.x);
	return Shadertoy::vFastLen3(position) - radius; // return position.Length() - radius;
}

static void RenderSpikeyMap_2x2_Distant(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	Vector3 fogColor(0.19f, 0.11f, 0.18f);

	const float speed = Rocket::getf(trackSpikeSpeed);
	const float roll = Rocket::getf(trackSpikeRoll);
	const float specPow = Rocket::getf(trackSpikeSpecular);

	fTest_global = Vector4(speed*time, 8.f, 16.f, 0.f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, kGoldenRatio); // FIXME: possible parameter

				Vector3 origin(0.f, 0.f, -3.314f);
				Vector3 direction(UV.x/kAspect, UV.y, 1.f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);
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
					march = fSpikey2(hit);
					total += march*(0.1f*kGoldenRatio);

					// enable if not screen-filling, otherwise it just costs a lot of cycles
//					if (total < 0.01f || total > 10.f)
//					{
//						break;
//					}
				}

				float nOffs = 0.1f;
				Vector3 normal(
					march-fSpikey2(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				const float specular = powf(std::max(0.f, normal*direction), specPow);
				float diffuse = normal.y*0.1f + normal.x*0.25f + normal.z*0.5f;

				const float distance = total;

				Vector3 color(diffuse);
				color += Shadertoy::CosPalSimplest(distance, 0.314f + 0.25f*diffuse, Vector3(0.2f, 0.1f, 0.6f), 0.5314f);
				color += specular+specular;

				__m128 desaturated = Shadertoy::Desaturate(color, 0.614f);
				__m128 fogged = Shadertoy::ApplyFog(distance, desaturated, fogColor, 0.0733f);

				colors[iColor] = Shadertoy::GammaAdj(fogged, 1.714f);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

static void RenderSpikeyMap_2x2_Distant_SpecularOnly(uint32_t *pDest, float time, float warmup)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const float speed = Rocket::getf(trackSpikeSpeed);
	const float roll = Rocket::getf(trackSpikeRoll);
	const float specPow = Rocket::getf(trackSpikeSpecular);

	fTest_global = Vector4(speed*time, 8.f, 16.f, 0.f);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, kGoldenRatio); // FIXME: possible parameter

				Vector3 origin(0.f, 0.f, -3.314f);
				Vector3 direction(UV.x/kAspect, UV.y, 1.f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);
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
					march = fSpikey2(hit);
					total += march*(0.1f*kGoldenRatio);

					// enable if not screen-filling, otherwise it just costs a lot of cycles
//					if (total < 0.01f || total > 10.f)
//					{
//						break;
//					}
				}

				float nOffs = 0.1f;
				Vector3 normal(
					march-fSpikey2(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				const float distance = hit.z-origin.z;

				const float specular = warmup*powf(std::max(0.f, normal*direction), specPow);
				__m128 fogged = Shadertoy::ApplyFog(distance, _mm_set1_ps(specular), _mm_setzero_ps(), 0.0733f);

				colors[iColor] = fogged;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Spikey_Draw(uint32_t *pDest, float time, float delta, bool close /* = true */)
{
	if (close)
	{
		RenderSpikeyMap_2x2_Close(g_pFxMap, time);
		Fx_Blit_2x2_Dithered(pDest, g_pFxMap);
	}
	else
	{
		const float warmup = Rocket::getf(trackDistSpikeWarmup);
		const bool specularOnly = 0.f != warmup;
		
		if (!specularOnly)
		{
			// render as normal
			RenderSpikeyMap_2x2_Distant(g_pFxMap, time);
			Fx_Blit_2x2(pDest, g_pFxMap);
		}
		else
		{
			// render only specular, can be used for a transition as seen in Aura for Laura (hence the track name "warmup")
			RenderSpikeyMap_2x2_Distant_SpecularOnly(g_pFxMap, time, 1.f+warmup);
			HorizontalBoxBlur32(g_pFxMap, g_pFxMap, kFxMapResX, kFxMapResY, warmup*0.001f*kGoldenRatio);
			Fx_Blit_2x2_Dithered(pDest, g_pFxMap);
		}
	}

}

//
// Classic free-directional tunnel (ported from my 2007 codebase).
// Test case for texture sampling and color interpolation versus UV interpolation.
//
// Idea for when this one works: https://www.shadertoy.com/view/MsX3RH
//

static void RenderTunnelMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	// FIXME: parametrize
	float boxy = 0.7f;
	float flowerScale = 0.14f;
	float flowerFreq = 5.f;
	float flowerPhase = time;
	float speed = 4.f;
	float roll = 0.f;

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 1.f);
				Vector3 direction(UV.x/kAspect, UV.y, 1.f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);

				// FIXME: this seems very suitable for SIMD, but the FPS is good as it is
				float A;
				A = direction.x*direction.x + direction.y*direction.y;
				A += flowerScale*lutsinf(atan2f(direction.y, direction.x)*flowerFreq + flowerPhase) - flowerScale;

				float absX = fabsf(direction.x);
				float absY = fabsf(direction.y);
				float box = absX > absY ? absX : absY;
				A = lerpf(A, box, boxy);
				A += kEpsilon;
				A = 1.f/A;
				float radius = 2.314f;
				float T = radius*A;
				Vector3 intersection = direction*T;

				float U = atan2f(intersection.y, intersection.x)/kPI;
				float V = intersection.z + time*speed;
				float shade = expf(-0.003f*T*T);

				int fpU = ftofp24(U*256.f);
				int fpV = ftofp24(V*8.f);

				unsigned U0, V0, U1, V1, fracU, fracV;
				bsamp_prepUVs(fpU, fpV, 255, 8, U0, V0, U1, V1, fracU, fracV);
				__m128 color = bsamp32_32f(s_pFDTunnelTex, U0, V0, U1, V1, fracU, fracV);

				color = _mm_mul_ps(color, _mm_set1_ps(shade));

				colors[iColor] = color;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4_NoConv(colors);
		}
	}
}

void Tunnel_Draw(uint32_t *pDest, float time, float delta)
{
	RenderTunnelMap_2x2(g_pFxMap, time);
	Fx_Blit_2x2(pDest, g_pFxMap);
}
