
// cookiedough -- simple Shadertoy ports & my own shaders

/*
	important (please read!):
		- when calculating UVs using the helper functions, you may want/need to apply kAspect again!
		- R and B are swapped, as in: Vector3 color(B, G, R)
		- Vector3 and Vector4 are 16-bit aligned and can cast to __m128 (SIMD) once needed (use '.vSSE')
		- when scaling a vector by a scalar in a loop, write it in place instead of using the operator (which won't inline for some reason)
		- if needed, parts of loops can be parallelized (SIMD), but that's a lot of hassle
		- an obvious optimization is to get offsets and deltas to calculate current UV, but that won't parallelize with OpenMP
		- be careful (precision, overflow) with normals and lighting calculations (see shadertoy-util.h)
		- OpenMP's dynamic scheduling is usually best since A) there's little cache penalty due to out-of-order and B) unbalanced load

	to do (priority):
		- optimize where you can (which does not always mean more SIMD)
		- fix normals left and right
*/

#include "main.h"
// #include "shadertoy.h"
#include "image.h"
#include "bilinear.h"
#include "shadertoy-util.h"
#include "boxblur.h"
#include "rocket.h"
#include "polar.h"

// --- Sync. tracks ---

// Laura sync. 
SyncTrack trackLauraSpeed;
SyncTrack trackLauraYaw, trackLauraPitch, trackLauraRoll;
SyncTrack trackLauraHue;
SyncTrack trackLauraSaturate;

// Nautilus sync.:
SyncTrack trackNautilusRoll;
SyncTrack trackNautilusBlur;
SyncTrack trackNautilusHue;
SyncTrack trackNautilusSpeed;

// Spike (close) sync.:
SyncTrack trackSpikeSpeed;
SyncTrack trackSpikeRoll;
SyncTrack trackSpikeSpecular;
SyncTrack trackSpikeDesaturation;
SyncTrack trackDistSpikeWarmup; // specular-only glow blur effect, yanked from Aura for Laura
SyncTrack trackSpikeHue;
SyncTrack trackSpikeGamma;
SyncTrack trackDistSpikeX;
SyncTrack trackDistSpikeY;
SyncTrack trackDistSpikeZ;
SyncTrack trackCloseSpikeX;
SyncTrack trackCloseSpikeY;

// Sinuses sync.:
SyncTrack trackSinusesSpecular;
SyncTrack trackSinusesRoll;
SyncTrack trackSinusesSpeed;

// Plasma sync.:
SyncTrack trackPlasmaSpeed;
SyncTrack trackPlasmaHue;
SyncTrack trackPlasmaGamma;

// --------------------

static uint32_t *s_pFDTunnelTex = nullptr;

bool Shadertoy_Create()
{
	// Aura for Laura:
	trackLauraSpeed = Rocket::AddTrack("laura:Speed");
	trackLauraYaw = Rocket::AddTrack("laura:Yaw");
	trackLauraPitch = Rocket::AddTrack("laura:Pitch");
	trackLauraRoll = Rocket::AddTrack("laura:Roll");
	trackLauraHue = Rocket::AddTrack("laura:Hue");
	trackLauraSaturate = Rocket::AddTrack("laura:Saturate");

	// Nautilus:
	trackNautilusRoll = Rocket::AddTrack("nautilus:Roll");
	trackNautilusBlur = Rocket::AddTrack("nautilus:Blur");
	trackNautilusHue = Rocket::AddTrack("nautilus:Hue");
	trackNautilusSpeed = Rocket::AddTrack("nautilus:Speed");

	// Spikes:
	trackSpikeSpeed = Rocket::AddTrack("spike:Speed");
	trackSpikeRoll = Rocket::AddTrack("spike:Roll");
	trackSpikeSpecular = Rocket::AddTrack("spike:Specular");
	trackSpikeDesaturation = Rocket::AddTrack("spike:Desaturation");
	trackDistSpikeWarmup = Rocket::AddTrack("distSpike:Warmup");
	trackSpikeHue = Rocket::AddTrack("spike:Hue");
	trackSpikeGamma = Rocket::AddTrack("spike:Gamma");
	trackDistSpikeX = Rocket::AddTrack("distSpike:xOffs");
	trackDistSpikeY = Rocket::AddTrack("distSpike:yOffs");
	trackDistSpikeZ = Rocket::AddTrack("distSpike:zOffs");
	trackCloseSpikeX = Rocket::AddTrack("closeSpike:xOffs");
	trackCloseSpikeY = Rocket::AddTrack("closeSpike:yOffs");

	// Sinuses:
	trackSinusesSpecular = Rocket::AddTrack("sinusesTunnel:Specular");
	trackSinusesRoll = Rocket::AddTrack("sinusesTunnel:Roll");
	trackSinusesSpeed = Rocket::AddTrack("sinusesTunnel:Speed");

	// Plasma:
	trackPlasmaSpeed = Rocket::AddTrack("plasma:Speed");
	trackPlasmaHue = Rocket::AddTrack("plasma:Hue");
	trackPlasmaGamma = Rocket::AddTrack("plasma:Gamma");

	s_pFDTunnelTex = Image_Load32("assets/shadertoy/grid2b.jpg");
	if (nullptr == s_pFDTunnelTex)
		return false;

	return true;
}

void Shadertoy_Destroy()
{
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
	return sqrtf(fX*fX + fY*fY + fZ*fZ)-0.8f;
//	return 1.f/Q3_rsqrtf(fX*fX + fY*fY + fZ*fZ)-0.8f;
}

static void RenderPlasmaMap(uint32_t *pDest, float time)
{
	const float speed = Rocket::getf(trackPlasmaSpeed);
	const float hue   = Rocket::getf(trackPlasmaHue);
	const float gamma = Rocket::getf(trackPlasmaGamma);

	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	// keeping as much out of the inner loop as possible, could not be effective in some cases (local variables, cache et cetera)
	const Vector3 colMulA = Shadertoy::MichielPal(hue);
	const Vector3 colMulB = Vector3(Shadertoy::Desaturate(colMulA, 0.8f));

	time = time*speed;

	const int cosIndex = tocosindex(time*0.314f*0.5f);
	const float dirCos = lutcosf(cosIndex);
	const float dirSin = lutsinf(cosIndex);

	#pragma omp parallel for schedule(dynamic)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;

		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				// idea: minus fifty gives a black bar on the left, ideal for an old school logo
//				const auto UV = Shadertoy::ToUV_FxMap(iX+iColor-50, iY, 4.f);
				const auto UV = Shadertoy::ToUV_FxMap(iX+iColor, iY, 4.f);

				const Vector3 direction(
					dirCos*UV.x*kAspect - dirSin*0.75f,
					UV.y,
					dirSin*UV.x + dirCos*0.75f);

				float total = 0.f, march;
				Vector3 hit(0.f);
				for (int iStep = 0; iStep < 24; ++iStep)
				{					
					march = fPlasma(hit, time);
//					if (march < 0.001f)
//						break;

					total += march*(0.5f*kGoldenRatio);

					hit.x = direction.x*total;
					hit.y = direction.y*total;
					hit.z = direction.z*total;
				}
				
				Vector3 color = colMulA*march + colMulB*fPlasma(hit*0.5f, time); 
				color *= 8.f - direction.x*0.5f;

				colors[iColor] = Shadertoy::GammaAdj(color, gamma);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Plasma_Draw(uint32_t *pDest, float time, float delta)
{
	RenderPlasmaMap(g_pFxMap[0], time);
	Fx_Blit_2x2(pDest, g_pFxMap[0]);
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
	const float roll = Rocket::getf(trackNautilusRoll);
	const float hue = Rocket::getf(trackNautilusHue);
	const float speed = Rocket::getf(trackNautilusSpeed);

	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	time = time*speed;

	fNautilus_global.x = time*0.125f;
	fNautilus_global.y = time/9.f;
	fNautilus_global.z = lutcosf(time*0.1428f);

	// slightly different from MichielPal() for some reason
	const Vector3 colorization(
		.1f-lutcosf(hue/3.f)/19.f, 
		.1f, 
		.1f+lutcosf(hue/14.f)/8.f);

	#pragma omp parallel for schedule(dynamic)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;

		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			const int destIndex = (yIndex+iX)>>2;

			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 2.f);

				Vector3 direction(UV.x*kAspect, UV.y, 1.f); 
				Shadertoy::rotZ(roll*time, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float total = 0.01f;
				float march = 1.f;
				for (int iStep = 0; march > 0.01f && iStep < 48; ++iStep)
				{
					hit.x = direction.x*total;
					hit.y = direction.y*total;
					hit.z = direction.z*total;
					
					march = fNautilus(hit, time);

					total += march*0.628f;
				}

				constexpr float nOffs = 0.15f;
				Vector3 normal(
					march-fNautilus(Vector3(hit.x+nOffs, hit.y, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y+nOffs, hit.z), time),
					march-fNautilus(Vector3(hit.x, hit.y, hit.z+nOffs), time));
				Shadertoy::vFastNorm3(normal);

				// I will leave the calculations here as written by Michiel (rust in vrede):
				float diffuse = normal.z*0.1f;
				float specular = powf(std::max(0.f, normal*direction), 16.f);

				Vector3 color(diffuse);
				color += colorization*(1.56f*total + specular);
				color += specular*0.314f;

				colors[iColor] = Shadertoy::GammaAdj(color, 2.22f);
			}

			pDest128[destIndex] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Nautilus_Draw(uint32_t *pDest, float time, float delta)
{
	RenderNautilusMap_2x2(g_pFxMap[0], time);

	const float blur = BoxBlurScale(Rocket::getf(trackNautilusBlur));
	if (0.f != blur)
	{
		Fx_Blit_2x2(pDest, g_pFxMap[0]);
		BoxBlur32(pDest, pDest, kResX, kResY, blur);
	}
	else
		Fx_Blit_2x2(pDest, g_pFxMap[0]);
}

//
// A few distorted spheres (spikes).
//
// FIXME:
// - try to "fix" normals
// - real specular please?
//

static Vector4 fSpike_global;

VIZ_INLINE float fSpikey1(const Vector3 &position) 
{
	constexpr float scale = kGoldenRatio*0.1f;
	const float radius = 1.35f + scale*lutcosf(fSpike_global.y*position.y - fSpike_global.x) + scale*lutcosf(fSpike_global.z*position.x + fSpike_global.x);
	return Shadertoy::vFastLen3(position) - radius; // return position.Length() - radius;
}

VIZ_INLINE float fSpikey2(const Vector3 &position) 
{
	constexpr float scale = kGoldenRatio*0.1f;
	const float radius = 1.35f + scale*lutcosf(fSpike_global.y*position.y - fSpike_global.x) + scale*lutcosf(fSpike_global.z*position.x + fSpike_global.x);
	return Shadertoy::vFastLen3(position) - radius; // return position.Length() - radius;
}

static void RenderSpikeyMap_2x2_Close(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const float speed = Rocket::getf(trackSpikeSpeed);
	const float roll = Rocket::getf(trackSpikeRoll);
	const float specPow = Rocket::getf(trackSpikeSpecular);
	const float desaturation = Rocket::getf(trackSpikeDesaturation);
	const float hue = Rocket::getf(trackSpikeHue);
	const float gamma = Rocket::getf(trackSpikeGamma);
	const float xOffs = Rocket::getf(trackCloseSpikeX);
	const float yOffs = Rocket::getf(trackCloseSpikeY);

	const Vector3 colorization = Shadertoy::MichielPal(hue);
	const __m128 diffColor = Shadertoy::Desaturate(colorization, desaturation);

	fSpike_global = Vector4(speed*time, 16.f, 22.f, 0.f);

	#pragma omp parallel for schedule(dynamic)
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
				Vector3 direction((UV.x+xOffs)*kAspect, UV.y + yOffs, 1.f); 
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

				colors[iColor] = Shadertoy::GammaAdj(Shadertoy::vLerp4(
					_mm_mul_ps(_mm_add_ps(diffColor, _mm_set1_ps(specular)), _mm_set1_ps(diffuse)), _mm_set1_ps(1.f), Shadertoy::ExpFog(distance, 0.314f)),
						gamma);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

static void RenderSpikeyMap_2x2_Distant(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const float speed = Rocket::getf(trackSpikeSpeed);
	const float roll = Rocket::getf(trackSpikeRoll);
	const float specPow = Rocket::getf(trackSpikeSpecular);
	const float desaturation = Rocket::getf(trackSpikeDesaturation);
	const float hue = Rocket::getf(trackSpikeHue);
	const float xOffs = Rocket::getf(trackDistSpikeX);
	const float yOffs = Rocket::getf(trackDistSpikeY);
	const float zOffs = Rocket::getf(trackDistSpikeZ);
	const float gamma = Rocket::getf(trackSpikeGamma);

	const Vector3 colorization = Shadertoy::MichielPal(hue);
	const __m128 diffColor = Shadertoy::Desaturate(colorization, desaturation); 

	fSpike_global = Vector4(speed*time, 16.f, 16.f, kEpsilon);

	const Vector3 origin(0.f, 0.f, -2.614f + zOffs);

	#pragma omp parallel for schedule(dynamic)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;

		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 2.f);
				
				Vector3 direction(UV.x + xOffs, UV.y + yOffs, 1.f); 
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

					total += march*0.314f;
				}

				constexpr float nOffs = 0.05f;
				Vector3 normal(
					march-fSpikey2(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				const float diffuse = std::max<float>(0.f, normal.z*0.8f + normal.y*0.2f);
				const float fakeSpecular = powf(std::max(0.25f, normal*direction), specPow);
				const float distance = hit.z-origin.z;
				
				colors[iColor] = Shadertoy::GammaAdj(Shadertoy::vLerp4(
					_mm_mul_ps(_mm_add_ps(diffColor, _mm_set1_ps(fakeSpecular)), _mm_set1_ps(diffuse)), _mm_set1_ps(1.f), Shadertoy::ExpFog(distance, 0.133f)),
					gamma);
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

	#pragma omp parallel for schedule(dynamic)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;

		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, kGoldenRatio);

				Vector3 origin(0.f, 0.f, -3.314f);
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rotZ(roll, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f; 
				for (int iStep = 0; iStep < 36; ++iStep)
				{
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;

					march = fSpikey2(hit);

					total += march*0.075f*kGoldenRatio;
				}

				constexpr float nOffs = 0.01f;
				Vector3 normal(
					march-fSpikey2(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSpikey2(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				const float distance = hit.z-origin.z;
				const float fakeSpecular = warmup*powf(std::max(0.f, normal*direction), specPow);

				const __m128 fogged = Shadertoy::vLerp4(_mm_set_ps1(fakeSpecular), _mm_setzero_ps(), Shadertoy::ExpFog(distance, 0.0333f));
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
		RenderSpikeyMap_2x2_Close(g_pFxMap[0], time);
		Fx_Blit_2x2(pDest, g_pFxMap[0]);
	}
	else
	{
		// render ball
		const float warmup = Rocket::getf(trackDistSpikeWarmup);
		const bool specularOnly = 0.f != warmup;
		
		if (!specularOnly)
		{
			// render as normal
			RenderSpikeyMap_2x2_Distant(g_pFxMap[0], time);
			Fx_Blit_2x2(pDest, g_pFxMap[0]);
		}
		else
		{
			// render only specular, can be used for a transition as seen in Aura for Laura (hence the track name "warmup")
			RenderSpikeyMap_2x2_Distant_SpecularOnly(g_pFxMap[0], time, 1.f+warmup);
//			HorizontalBoxBlur32(g_pFxMap[1], g_pFxMap[0], kFxMapResX, kFxMapResY, warmup*0.005f*kGoldenRatio);
			Fx_Blit_2x2(g_renderTarget[0], g_pFxMap[0]);
			HorizontalBoxBlur32(pDest, g_renderTarget[0], kResX, kResY, warmup*0.005f*kGoldenRatio);
		}
	}

}

//
// Classic free-directional tunnel (ported from my 2007 demoscene codebase).
// Test case for texture sampling and color interpolation versus UV interpolation.
//
// FIXME:
// - expected (hardcoded): 256x256 texture
// - parametrize
// - fake project light, like in the Mewlers 64KB 'Viagra'?
//

static void RenderTunnelMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	// FIXME: parametrize
	const float boxy = 0.6f * (0.5f + sinf(time)*0.5f);
	const float flowerScale = 0.05f;
	const float flowerFreq = 6.f;
	const float flowerPhase = time;
	const float speed = 4.f;
	const float roll = time*0.6f;
	const float radius = 3.f;

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;

		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 4.f);
				Vector3 direction(UV.x, UV.y, 1.f); 
				Shadertoy::rotX(time*0.3f, direction.y, direction.z);
				Shadertoy::rotZ(roll, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				// FIXME: this seems very suitable for SIMD, but the FPS is good as it is
				float A;
				A = direction.x*direction.x + direction.y*direction.y;
				A += flowerScale*lutcosf(atan2f(direction.y, direction.x)*flowerFreq + flowerPhase);

				float absX = fabsf(direction.x);
				float absY = fabsf(direction.y);
				float box = absX > absY ? absX : absY;
				A = lerpf(A, box, boxy); //  smoothstepf(A, box, boxy);
				A += kEpsilon;
				A = 1.f/A;
				float T = radius*A;
				Vector3 intersection = direction*T;

				float U = atan2f(intersection.y, intersection.x)/kPI;
				float V = intersection.z + time*speed;
				float shade = 1.f-expf(-0.004f*T*T);
				int fpU = ftofp24(U*512.f);
				int fpV = ftofp24(V*8.f);

				unsigned U0, V0, U1, V1, fracU, fracV;
				bsamp_prepUVs(fpU, fpV, 255, 8, U0, V0, U1, V1, fracU, fracV);
				__m128 color = bsamp32_32f(s_pFDTunnelTex, U0, V0, U1, V1, fracU, fracV);

				color = Shadertoy::vLerp4(color, _mm_set1_ps(255.f), shade);

				colors[iColor] = color;
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4_NoConv(colors);
		}
	}
}

void Tunnel_Draw(uint32_t *pDest, float time, float delta)
{
	RenderTunnelMap_2x2(g_pFxMap[0], time);
	Fx_Blit_2x2(g_renderTarget[0], g_pFxMap[0]);
	HorizontalBoxBlur32(pDest, g_renderTarget[0], kResX, kResY, 3.f*kBoxBlurScale);
}

//
// A blobby grid; stole the idea of forming an object around the camera path from Shadertoy's Shane.
// My Shadertoy version: https://www.shadertoy.com/view/XldyzX (private)
//
// It's glitchy, it's grainy, but with the right parameters and colors might be useful for a short show. 
//
// FIXME:
// - try to "fix" normals
//

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

static void RenderSinMap_2x2(uint32_t *pDest, float time)
{
	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	const float specPow = 1.f + Rocket::getf(trackSinusesSpecular);
	const float roll = Rocket::getf(trackSinusesRoll);
	const float speed = Rocket::getf(trackSinusesSpeed);
	const float fog = 0.03f; // (FIXME: was 0.224f, parametrize!)

	const Vector3 diffColor(0.15f, 0.6f, 0.8f);

	const Vector3 origin = fSinPath(time*speed);

	#pragma omp parallel for schedule(dynamic)
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;

		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];

			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY, 2.f); 

				Vector3 direction(UV.x*kAspect, UV.y, 0.3f); 
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

				constexpr float nOffs = 0.2f;
				Vector3 normal(
					march-fSinMap(Vector3(hit.x+nOffs, hit.y, hit.z)),
					march-fSinMap(Vector3(hit.x, hit.y+nOffs, hit.z)),
					march-fSinMap(Vector3(hit.x, hit.y, hit.z+nOffs)));
				Shadertoy::vFastNorm3(normal);

				float diffuse = normal.z*0.7f + 0.3f*normal.y;
				diffuse = 0.2f + 0.8f*diffuse;

				const float specDot = normal*direction; // std::max(0.1f, normal*direction);
				const float fakeSpecular = powf(specDot, specPow);

				const float distance = hit.z-origin.z;

				colors[iColor] = Shadertoy::GammaAdj(Shadertoy::vLerp4(
					_mm_mul_ps(_mm_add_ps(diffColor, _mm_set1_ps(fakeSpecular)), _mm_set1_ps(diffuse)), _mm_set1_ps(1.f), Shadertoy::ExpFog(distance, fog)),
					2.73f);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Sinuses_Draw(uint32_t *pDest, float time, float delta)
{
	RenderSinMap_2x2(g_pFxMap[0], time);
	Fx_Blit_2x2(pDest, g_pFxMap[0]);
}

//
// Prototype for 'Aura for Laura' cosine grid shader.
// Preferably to be finished by a certain mr. Ernst Hot.
//
// In the end it ended up nothing like it, right now it's more like weird speaker design gone wrong.
// Best viewed with values like 1.0, 0.2, 0.2, 0.1 respectively, for example.
//

VIZ_INLINE float fLaura(const Vector3 &position)
{
	return lutcosf(position.x)+lutcosf(position.y)+lutcosf(position.z) + 1.f;
}

VIZ_INLINE const Vector3 LauraNormal(float march, const Vector3 &hit)
{
	constexpr float nOffs = 0.1628f;

	Vector3 normal(
		fLaura(Vector3(hit.x+nOffs, hit.y, hit.z))-march,
		fLaura(Vector3(hit.x, hit.y+nOffs, hit.z))-march,
		fLaura(Vector3(hit.x, hit.y, hit.z+nOffs))-march);

	Shadertoy::vFastNorm3(normal);

	return normal;
}

void RenderLaura_2x2(uint32_t *pDest, float time)
{
	const float lauraSpeed = Rocket::getf(trackLauraSpeed);
	const float lauraYaw = Rocket::getf(trackLauraYaw);
	const float lauraPitch = Rocket::getf(trackLauraPitch);
	const float lauraRoll = Rocket::getf(trackLauraRoll);
	const float hue = Rocket::getf(trackLauraHue);
	const float saturate = Rocket::getf(trackLauraSaturate);

	__m128i *pDest128 = reinterpret_cast<__m128i*>(pDest);

	/* const */ Vector3 colorization = Shadertoy::MichielPal(hue);
	const __m128 diffColor = Shadertoy::Desaturate(colorization, saturate); // over-saturate a bit!

	Vector3 origin(0.f, 0.f, lauraSpeed*time);

	#pragma omp parallel for schedule(dynamic) 
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		const int yIndex = iY*kFxMapResX;

		for (int iX = 0; iX < kFxMapResX; iX += 4)
		{	
			__m128 colors[4];
			for (int iColor = 0; iColor < 4; ++iColor)
			{
				auto UV = Shadertoy::ToUV_FxMap(iColor+iX, iY);

				Vector3 direction(UV.x*kAspect, UV.y, kPI); 
				Shadertoy::rotY(lauraYaw, direction.x, direction.z);
				Shadertoy::rotX(lauraPitch, direction.y, direction.z);
				Shadertoy::rotZ(lauraRoll*time, direction.x, direction.y);
				Shadertoy::vFastNorm3(direction);

				Vector3 hit;

				float march, total = 0.f; 
				int iStep;
				for (iStep = 0; iStep < 32; ++iStep)
				{
					hit.x = origin.x + direction.x*total;
					hit.y = origin.y + direction.y*total;
					hit.z = origin.z + direction.z*total;
					
					march = fLaura(hit);
					
					total += march*0.5f;
				}

				/*
				constexpr float nOffs = 0.0628f;
				Vector3 normal(
					fLaura(Vector3(hit.x+nOffs, hit.y, hit.z))-march,
					fLaura(Vector3(hit.x, hit.y+nOffs, hit.z))-march,
					fLaura(Vector3(hit.x, hit.y, hit.z+nOffs))-march);
				Shadertoy::vFastNorm3(normal);
				*/

				const Vector3 normal = LauraNormal(march, hit);

				const Vector3 lightPos = origin - direction;
				Vector3 lightDir = (lightPos-hit);
				Shadertoy::vFastNorm3(lightDir);

				float diffuse = std::max<float>(0.3f, normal*lightDir);
				const float distance = hit.z-origin.z;
				const float specular = Shadertoy::Specular(origin, hit, normal, lightDir, 4.f);

				float rim = diffuse*diffuse;
				rim = (rim*rim-0.13f)*32.f;
				rim = std::max<float>(1.f, std::min<float>(0.f, rim));
				diffuse *= rim;

//				colors[iColor] = Shadertoy::GammaAdj(Shadertoy::vLerp4(
//					_mm_mul_ps(_mm_add_ps(diffColor, _mm_set1_ps(specular)), _mm_set1_ps(diffuse)), _mm_set1_ps(1.f), Shadertoy::ExpFog(distance, 0.0001f)),
//					2.73f);

				colors[iColor] = Shadertoy::GammaAdj(Shadertoy::vLerp4(
					_mm_mul_ps(diffColor, _mm_set1_ps(diffuse+specular)), _mm_set1_ps(Q3_rsqrtf(specular+diffuse)), Shadertoy::ExpFog(distance, 0.001f)),
					2.33f);
			}

			const int index = (yIndex+iX)>>2;
			pDest128[index] = Shadertoy::ToPixel4(colors);
		}
	}
}

void Laura_Draw(uint32_t *pDest, float time, float delta)
{
	RenderLaura_2x2(g_pFxMap[0], time);
	Fx_Blit_2x2(pDest, g_pFxMap[0]);
}


