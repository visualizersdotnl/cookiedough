
// cookiedough -- simple Shadertoy ports & my own shaders

/*
	important:
		- *** R and B are swapped, as in: Vector3 color(B, G, R) ***
		- Vector3 and Vector4 are 16-bit aligned and can cast to __m128 (SIMD) once needed
		- the "to UV" functions in shader-util.h take aspect ratio into account, you don't always want this (see spikey objects for example)
		  in that case divide UV.x by kAspect
		- when scaling a vector by a scalar in a loop, write it in place instead of using the operator (which won't inline for some reason)
		- if needed, parts of loops can be parallelized (SIMD), but that's a lot of hassle
		- an obvious optimization is to get offsets and deltas to calculate current UV, but that won't parallelize with OpenMP
		- be careful (precision, overflow) with normals and lighting calculations (see shadertoy-util.h)

	to do:
		- optimize where you can (which does not always mean more SIMD)
		- hardcoded colors should be tweakable through assets
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
SyncTrack trackSpikeDesaturation;
SyncTrack trackDistSpikeWarmup; // specular-only glow blur effect, yanked from Aura for Laura

// Sinuses sync.:
SyncTrack trackSinusesBrightness;
SyncTrack trackSinusesRoll;
SyncTrack trackSinusesSpeed;

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
	trackSpikeDesaturation = Rocket::AddTrack("cSpikeDesaturation");
	trackDistSpikeWarmup = Rocket::AddTrack("cDistSpikeWarmup");

	// Sinuses:
	trackSinusesBrightness = Rocket::AddTrack("sinusesBrightness");
	trackSinusesRoll = Rocket::AddTrack("sinusesRoll");
	trackSinusesSpeed = Rocket::AddTrack("sinusesSpeed");

	s_pFDTunnelTex = Image_Load32("assets/shadertoy/grid2b.jpg");
	if (nullptr == s_pFDTunnelTex)
		return false;

	return true;
}

void Shadertoy_Destroy()
{
}

VIZ_INLINE Vector3 Michiel_Kleuren(float time) {
	return Vector3(
		.1f-lutcosf(time/3.f)/(19.f*0.5f), 
		.1f, 
		.1f+lutcosf(time/14.f)/4.f);
}

//
// Plasma (see https://www.shadertoy.com/view/ldSfzm)
//
// Not a very interesting effect but if someone tweaks it it might be watchable for a couple of seconds.
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

	const Vector3 colMulA(0.1f, 0.3f, 0.6f);
	const Vector3 colMulB(0.05f, 0.1f, 0.2f);

	const float t = time*0.77f;

	#pragma omp parallel for schedule(dynamic, 1)
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

				const int cosIndex = tocosindex(t*0.314f*0.5f);
				const float dirCos = lutcosf(cosIndex);
				const float dirSin = lutsinf(cosIndex);

				Vector3 direction(
					dirCos*UV.x - dirSin*0.75f,
					UV.y,
					dirSin*UV.x + dirCos*0.75f);

				Vector3 origin = direction;
				for (int step = 0; step < 34; ++step)
				{
					float march = fPlasma(origin, t);
					if (fabsf(march) < 0.001f)
						break;

					origin.x += direction.x*march;
					origin.y += direction.y*march;
					origin.z += direction.z*march;
				}
				
				Vector3 color = colMulA*fPlasma(origin+direction, t) + colMulB*fPlasma(origin*0.5f, t); 
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
}

//
// Nautilus Redux by Michiel v/d Berg
// 
// Definitely a keeper. He was OK with that.
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

	#pragma omp parallel for schedule(dynamic, 1) collapse(2)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			const int yIndex = iY*kFxMapResX;

			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 2.f); // FIXME: possible parameter

				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float total = 0.01f;
				float march = 1.f;
				for (int iStep = 0; fabsf(march)> 0.01f && iStep < 64; ++iStep)
				{
					hit.x = direction.x*total;
					hit.y = direction.y*total;
					hit.z = direction.z*total;
					march = fNautilus(hit, time);

					total += march*0.628f;
				}

				float nOffs = 0.15f;
				Vector3 normal(
					march-fNautilus(Vector3(hit.x+nOffs, hit.y, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y+nOffs, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y, hit.z+nOffs), time));
				Shadertoy::vFastNorm3(normal);

				// I will leave the calculations here as made by Michiel:

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
// - make it look like competition shader
// - optimize
//

// cosine blob tunnel
VIZ_INLINE float fAuraForLaura(const Vector3 &position)
{	
	const float grid = fastcosf(position.x)+fastcosf(position.y)+fastcosf(position.z);
 
//	const Vector3 cosPos(fastcosf(position.x), fastcosf(position.y), fastcosf(position.z));
// 	const Vector3 sinSwizzled(fastsinf(position.z), fastsinf(position.x), fastsinf(position.y));
//	const float grid = cosPos*sinSwizzled + 1.f;

	return grid;
}

VIZ_INLINE const Vector3 fLauraPath(float Z)
{
	return { 0.5f*fastsinf(Z*3.33f), 0.624f*fastcosf(Z*0.53f), Z*4.f };
}

static void RenderLauraMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const float lauraZ = time*Rocket::getf(trackLauraSpeed);
	const float lauraYaw = Rocket::getf(trackLauraYaw);
	const float lauraPitch = Rocket::getf(trackLauraPitch);
	const float lauraRoll = Rocket::getf(trackLauraRoll);

	Vector3 fogColor(0.7f, 0.8f, 0.1f);
	fogColor *= 0.1f;

	const Vector3 origin = Vector3(fLauraPath(lauraZ));

	const float zebraCos1 = lutcosf(time*0.3f);
	const float zebraCos2 = lutcosf(time*0.414f);

/*
	// FIXME: parametrize (or at least a blend between sets)
	const Vector3 colorization(
		.1f-lutcosf(time/3.f)/(19.f*0.5f), 
		.1f, 
		.1f+lutcosf(time/14.f)/4.f);
*/
	Vector3 colorization = Michiel_Kleuren(time);

	#pragma omp parallel for schedule(dynamic, 1)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 64.f*(1920.f/1080.f)); // FIXME: possible parameter

				Vector3 direction(UV.x, UV.y, 2.314f); 
				
				// FIXME: use 3x3 matrix?
				Shadertoy::rotY(lauraYaw,   direction.x, direction.z);
				Shadertoy::rotX(lauraPitch, direction.y, direction.z);
				Shadertoy::rotZ(lauraRoll,  direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;
				float march, total = 0.f;
				for (int iStep = 0; iStep < 32; ++iStep)
				{
					hit = origin + direction*total;
					march = fAuraForLaura(hit);
					total += march*0.43f;

					if (fabsf(march) < 0.001f)
						break;
				}

				float nOffs = 0.1f;
				Vector3 normal(
					march-fAuraForLaura(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fAuraForLaura(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fAuraForLaura(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				// const Vector3 lightPos = Vector3(direction.x+origin.x, direction.y+origin.y, -origin.z);
				// Vector3 lightDir = Vector3(lightPos - hit);
				// Shadertoy::vFastNorm3(lightDir);
				// float diffuse = std::max(0.f, normal*lightDir); // normal*Vector3(0.f, 0.f, 1.f); // normal.y * 0.12f + normal.x * 0.12f + normal.z * 0.65f;
				// float diffuse = normal.y * 0.42f + normal.x * 0.12f + normal.z * 0.65f;
				// const float fresnel = powf(std::max(0.f, normal*lightDir), 16.f); // important one!
				
				float diffuse = normal.z*0.1f;
				float specular = powf(std::max(0.f, normal*direction), 16.f);

				// zebra! ish...
				Vector3 zebraHit = hit + Vector3(zebraCos1); // FIXME: scalar addition would work too? check library
				Shadertoy::vFastNorm3(zebraHit);
				float yMod = 0.5f + 0.5f*lutsinf(zebraHit.y*4.f + zebraHit.x*22.f + zebraHit.y+zebraCos2);
				diffuse *= yMod;
//				yMod *= kPI*2.f; yMod *= yMod;
//				diffuse += yMod;

				Vector3 color(diffuse);
				color = colorization*(diffuse+total + specular);
				color += specular*0.314f;

				colors[iColor] = Shadertoy::GammaAdj(color, 2.22f);

//				const float distance = hit.z-origin.z;

//				colors[iColor] = Shadertoy::CompLighting(diffuse, fresnel, distance, .05f, 1.f, colorization, fogColor);
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

static Vector4 fSpike_global;
VIZ_INLINE float fSpikey1(const Vector3 &position) 
{
	constexpr float scale = kGoldenRatio*0.1f;
	const float radius = 1.35f + scale*lutcosf(fSpike_global.y*position.y - fSpike_global.x) + scale*lutcosf(fSpike_global.z*position.x + fSpike_global.x);
	return Shadertoy::vFastLen3(position) - radius; // return position.Length() - radius;
}

static void RenderSpikeyMap_2x2_Close(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 fogColor(1.f);

	const float speed = Rocket::getf(trackSpikeSpeed);
	const float roll = Rocket::getf(trackSpikeRoll);
	const float specPow = Rocket::getf(trackSpikeSpecular);
	const float desaturation = Rocket::getf(trackSpikeDesaturation);

	/* const */ Vector3 colorization = Michiel_Kleuren(time);
	const __m128 diffColor = Shadertoy::Desaturate(colorization, desaturation);

	fSpike_global = Vector4(speed*time, 16.f, 22.f, 0.f);

	#pragma omp parallel for schedule(dynamic, 1) collapse(2)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 2.f); // FIXME: possible parameter

				Vector3 origin(0.2f, 0.f, -2.23f); // FIXME: nice parameters too!
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f; 
				int iStep;
				for (iStep = 0; iStep < 32; ++iStep)
				{
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;

					march = fSpikey1(hit);

					// enable if not screen-filling, otherwise it just costs a lot of cycles
//					if (total < 0.01f || total > 10.f)
//					{
//						break;
//					}

					total += march*0.14f;
				}

				constexpr float nOffs = 0.15f;
				Vector3 normal(
					march-fSpikey1(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSpikey1(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSpikey1(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				const float diffuse = normal.z;
				const float specular = powf(std::max(0.f, normal*direction), specPow);
				const float distance = hit.z-origin.z;
				colors[iColor] = Shadertoy::CompLighting(diffuse, specular, distance, 0.314f, 1.628f, diffColor, fogColor);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

VIZ_INLINE float fSpikey2(const Vector3 &position) 
{
	constexpr float scale = kGoldenRatio*0.1f;
	const float radius = 1.35f + scale*lutcosf(fSpike_global.y*position.y - fSpike_global.x) + scale*lutcosf(fSpike_global.z*position.x + fSpike_global.x);
	return Shadertoy::vFastLen3(position) - radius; // return position.Length() - radius;
}

static void RenderSpikeyMap_2x2_Distant(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const Vector3 fogColor(1.f, 1.f, 1.f);

	const float speed = Rocket::getf(trackSpikeSpeed);
	const float roll = Rocket::getf(trackSpikeRoll);
	const float specPow = Rocket::getf(trackSpikeSpecular);
	const float desaturation = Rocket::getf(trackSpikeDesaturation);

	Vector3 colorization = Michiel_Kleuren(time);

	fSpike_global = Vector4(speed*time, 8.f, 16.f, 0.f);

	#pragma omp parallel for schedule(dynamic, 1)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, kGoldenRatio); // FIXME: possible parameter

				Vector3 origin(0.f, 0.f, -2.614f);
				Vector3 direction(UV.x/kAspect, UV.y, 1.f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f; 
				int iStep;
				for (iStep = 0; iStep < 36; ++iStep)
				{
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;
					march = fSpikey2(hit);

					// enable if not screen-filling, otherwise it just costs a lot of cycles
//					if (total < 0.01f || total > 10.f)
//					{
//						break;
//					}

					total += march*0.314f; // (0.1f*kGoldenRatio);
				}

				float nOffs = 0.15f;
				Vector3 normal(
					march-fSpikey2(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				const float diffuse = normal.z*0.8f + normal.y*0.2f;
				const float specular = powf(std::max(0.25f, normal*direction), specPow);
				const float distance = hit.z-origin.z;
				__m128 diffColor = Shadertoy::Desaturate(colorization, desaturation); // Shadertoy::CosPalSimplest(distance, 1.f, Vector3(0.3f, 0.3f, 0.75f), 0.5314f), desaturation);
				colors[iColor] = Shadertoy::CompLighting(diffuse, specular, distance, 0.133f, 1.214f, diffColor, fogColor);
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

	fSpike_global = Vector4(speed*time, 8.f, 16.f, 0.f);

	#pragma omp parallel for schedule(dynamic, 1)
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
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;
					march = fSpikey2(hit);

					// enable if not screen-filling, otherwise it just costs a lot of cycles
//					if (total < 0.01f || total > 10.f)
//					{
//						break;
//					}

					total += march*(0.1f*kGoldenRatio);
				}

				float nOffs = 0.01f;
				Vector3 normal(
					march-fSpikey2(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				const float distance = hit.z-origin.z;
				const float specular = warmup*powf(std::max(0.f, normal*direction), specPow);

				__m128 fogged = Shadertoy::ApplyFog(distance, _mm_set1_ps(specular), _mm_setzero_ps(), 0.0433f);
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
		// render close-up
		RenderSpikeyMap_2x2_Close(g_pFxMap, time);
		Fx_Blit_2x2(pDest, g_pFxMap);
	}
	else
	{
		// render ball
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
			Fx_Blit_2x2(pDest, g_pFxMap);
		}
	}

}

//
// Classic free-directional tunnel (ported from my 2007 codebase).
// Test case for texture sampling and color interpolation versus UV interpolation.
//
// FIXME:
// - make it into some kind of flowery effect (like in Stars) that does not look this terrible
//

static void RenderTunnelMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	// FIXME: parametrize
	float boxy = 0.6f;
	float flowerScale = 0.0314f;
	float flowerFreq = 3.f;
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
				A = lerpf(A, box, boxy); //  smoothstepf(A, box, boxy);
				A += kEpsilon;
				A = 1.f/A;
				float radius = 1.f;
				float T = radius*A;
				Vector3 intersection = direction*T;

				float U = atan2f(intersection.y, intersection.x)/kPI;
				float V = intersection.z + time*speed;
				float shade = expf(-0.003f*T*T);

				int fpU = ftofp24(U*256.f);
				int fpV = ftofp24(V*16.f);

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

//
// A blobby grid; stole the idea of forming an object around the camera path from Shadertoy's Shane.
// My Shadertoy version: https://www.shadertoy.com/view/XldyzX (private)
//
// It's glitchy, it's grainy, but with the right parameters and colors might be useful for a short show. 
//

#include "q3-rsqrt.h"

VIZ_INLINE const Vector3 fSinPath(float time)
{
	const float timeMod = time*0.314f;
	const float sine = lutsinf(timeMod);
	const float cosine = lutcosf(timeMod);
	return { sine*2.f*kGoldenRatio - cosine*1.5f, cosine*3.14f + sine*kGoldenRatio, time };
}

// FIXME: try a SIMD version?
VIZ_INLINE float fSinMap(const Vector3 &point)
{
	float pZ = point.z;

	const float zMod = pZ*0.314f;
	const int cosIndex = tocosindex(zMod);
	const float pathCosine = lutcosf(cosIndex);
	const float pathSine = lutcosf(cosIndex+kCosTabSinPhase)*kGoldenRatio;
	float pX = point.x-(pathSine*2.f - pathCosine*1.5f);
	float pY = point.y-(pathCosine*3.14f + pathSine);

	float aX = pX*0.315f*1.25f + lutsinf(pZ*(0.814f*1.25f));
	float aY = pY*0.315f*1.25f + lutsinf(pX*(0.814f*1.25f));
	float aZ = pZ*0.315f*1.25f + lutsinf(pY*(0.814f*1.25f));

	float cosX = lutcosf(aX);
	float cosY = lutcosf(aY);
	float cosZ = lutcosf(aZ);

	float length = sqrtf(cosX*cosX + cosY*cosY + cosZ*cosZ);
	return (length - 1.025f)*1.33f;
}

/*
VIZ_INLINE float fSinShadow(Vector3 position, const Vector3 &direction, unsigned maxSteps)
{
	float result = 1.f;

	float march;
	float total = 0.1f;
	for (unsigned iStep = 0; iStep < maxSteps; ++iStep)
	{
		position += direction*total;
		march = fSinMap(position);
		if (march < 0.001f)
			return 0.f;

		result = std::min(result, 16.f*march/total);
		total += march;
	}

	return result;
}
*/

static void RenderSinMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const float brightness = Rocket::getf(trackSinusesBrightness);
	const float roll = Rocket::getf(trackSinusesRoll);
	const float speed = Rocket::getf(trackSinusesSpeed);

	const Vector3 diffColor(0.15f, 0.6f, 0.8f);
	const __m128 fogColor = _mm_set1_ps(1.f);

	const Vector3 origin = fSinPath(time*speed);
//	const Vector3 light = Vector3(origin.x, origin.y, origin.z+0.f);

//	#pragma omp parallel for schedule(dynamic, 1)
	#pragma omp parallel for schedule(dynamic, 1) collapse(2)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			const int yIndex = iY*kFxMapResX;
			__m128 colors[4];

			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 2.f); // FIXME: possible parameter

				Vector3 direction(UV.x/kAspect, UV.y, 0.314f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march = 1.f, total = 0.f;
				for (int iStep = 0; march > 0.01f && iStep < 32; ++iStep)
				{
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;

					march = fSinMap(hit);
	
					total += march*0.814f;
				}

				constexpr float nOffs = 0.1f;
				Vector3 normal(
					march-fSinMap(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSinMap(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSinMap(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

//				Vector3 lightDir = hit-light;
//				Shadertoy::vFastNorm3(lightDir);

//				float diffuse = normal*lightDir;
				float diffuse = normal.z*0.7f + 0.3f*normal.y;
				float specular = normal*direction; // std::max(0.1f, normal*direction);

				// const float shadow = fSinShadow(hit, normal, 14);
				diffuse = 0.2f + 0.8f*diffuse; //  + 0.3f*shadow;
				specular = powf(specular, 1.f+brightness);

				const float distance = hit.z-origin.z;
				colors[iColor] = Shadertoy::CompLighting(diffuse, specular, distance, 0.224f, 2.73f, diffColor, fogColor);
//				colors[iColor] = Shadertoy::CompLighting_MonoSpec(diffuse, specular, distance, 0.224f, 2.73f, diffColor, fogColor);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Sinuses_Draw(uint32_t *pDest, float time, float delta)
{
	RenderSinMap_2x2(g_pFxMap, time);
	Fx_Blit_2x2(pDest, g_pFxMap);
}


