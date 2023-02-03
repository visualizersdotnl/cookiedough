
// cookiedough -- voxel ball (2-pass approach)

#include "main.h"
// #include "ball.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
#include "boxblur.h"
#include "polar.h"
#include "voxel-shared.h"
#include "rocket.h"
// #include "shadertoy-util.h"

static uint8_t *s_pHeightMap[2] = { nullptr };
static uint32_t *s_pColorMap = nullptr;
static uint32_t *s_pBeamMap = nullptr;
static uint32_t *s_pEnvMap = nullptr;
static uint8_t *s_heightMapMix = nullptr;

// --- Sync. tracks ---

SyncTrack trackBallBlur;
SyncTrack trackBallRadius; 
SyncTrack trackBallRayLength;
SyncTrack trackBallSpikes;
SyncTrack trackBallHasBeams;

// --------------------
	
// -- voxel renderer --

// my notes about the current situation:
// - fix environment mapping & very basic lighting (shouldn't be too hard to derive a normal)
// - try to add a background: render the effect to separate target, then impose on background
// - try to move the object around
// - orange were doing something to curtail the beams, figure out what

#define NO_ENV_MAP

// adjust to map resolution
constexpr unsigned kMapSize = 512;
constexpr unsigned kMapAnd = kMapSize-1;                                          
constexpr unsigned kMapShift = 9;

// max. trace depth
constexpr unsigned kMaxRayLength = 1024;

// height projection table
static unsigned s_heightProj[kMaxRayLength];
static unsigned s_curRayLength = kMaxRayLength;

// max. radius (in pixels)
constexpr float kMaxBallRadius = 900.f;

// scale applied to each beam sample
constexpr auto kBeamMul = 4;

// how much (indexing gradient) to darken the beam while it's extruded
constexpr auto kBeamExtMul = 3;

static void vball_ray_beams(uint32_t *pDest, int curX, int curY, int dX, int dY, __m128i bgTint)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE16(s_pColorMap[U+V]);

	__m128i beamAccum = _mm_setzero_si128();
	const __m128i beamMul = g_gradientUnp[kBeamMul];
 	const __m128i beamExtMul = g_gradientUnp[kBeamExtMul];

	int envU = (kMapSize>>1)<<8;
	int envV = envU;

	for (unsigned int iStep = 0; iStep < s_curRayLength; ++iStep)
	{
		// I had the mapping the wrong way around (or I am adjusting for something odd elsewhere, but it works for now (FIXME))
		curX -= dX;
		curY -= dY;

#if !defined(NO_ENV_MAP)

		envU -= dX;
		envV -= dY;

#endif

		// prepare UVs
		unsigned int U0, V0, U1, V1, fracU, fracV;
		bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);

		// fetch height & unpacked color
		const unsigned int mapHeight = bsamp8(s_heightMapMix, U0, V0, U1, V1, fracU, fracV);
		__m128i color = bsamp32_16(s_pColorMap, U0, V0, U1, V1, fracU, fracV);

		// and beam (extrusion) color (while the prepared UVs are still intact)
		const __m128i beam = bsamp32_16(s_pBeamMap, U0, V0, U1, V1, fracU, fracV);

		// accumulate & add beam (separate map)
		beamAccum = _mm_adds_epu16(beamAccum, _mm_srli_epi16(_mm_mullo_epi16(beam, beamMul), 8));
		color = _mm_adds_epu16(color, beamAccum);

#if !defined(NO_ENV_MAP)

		// sample env. map
		// const unsigned int U = envU >> 8 & kMapAnd, V = (envV >> 8 & kMapAnd) << kMapShift;
		bsamp_prepUVs(envU, envV, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
		const __m128i additive = bsamp32_16(s_pEnvMap, U0, V0, U1, V1, fracU, fracV);
		color = _mm_adds_epu16(color, additive);

#endif

		// project height
		const unsigned int height = mapHeight*s_heightProj[iStep] >> 8;

		// voxel visible?
		if (height > lastDrawnHeight)
		{
			const unsigned int drawLength = height - lastDrawnHeight;

			// draw span (clipped)
			cspanISSE16(pDest + lastDrawnHeight, 1, height - lastHeight, drawLength, lastColor, color);
			lastDrawnHeight = height;
		}

		lastHeight = height;
		lastColor = color;
	}

	// beam fade out
	const unsigned int remainder = kTargetResX-lastDrawnHeight;
	cspanISSE16_noclip(pDest + lastDrawnHeight, 1, remainder, beamAccum, bgTint); 
}

static void vball_ray_no_beams(uint32_t *pDest, int curX, int curY, int dX, int dY, __m128i bgTint)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE16(s_pColorMap[U+V]);

	int envU = (kMapSize>>1)<<8;
	int envV = envU;

	for (unsigned int iStep = 0; iStep < s_curRayLength; ++iStep)
	{
		// I had the mapping the wrong way around (or I am adjusting for something odd elsewhere, but it works for now (FIXME))
		curX -= dX;
		curY -= dY;

#if !defined(NO_ENV_MAP)

		envU -= dX;
		envV -= dY;

#endif

		// prepare UVs
		unsigned int U0, V0, U1, V1, fracU, fracV;
		bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);

		// fetch height & unpacked color
		const unsigned int mapHeight = bsamp8(s_heightMapMix, U0, V0, U1, V1, fracU, fracV);
		__m128i color = bsamp32_16(s_pColorMap, U0, V0, U1, V1, fracU, fracV);

#if !defined(NO_ENV_MAP)

		// sample env. map
		// const unsigned int U = envU >> 8 & kMapAnd, V = (envV >> 8 & kMapAnd) << kMapShift;
		bsamp_prepUVs(envU, envV, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
		const __m128i additive = bsamp32_16(s_pEnvMap, U0, V0, U1, V1, fracU, fracV);
		color = _mm_adds_epu16(color, additive);

#endif

		// project height
		const unsigned int height = mapHeight*s_heightProj[iStep] >> 8;

		// voxel visible?
		if (height > lastDrawnHeight)
		{
			const unsigned int drawLength = height - lastDrawnHeight;

			// draw span (clipped)
			cspanISSE16(pDest + lastDrawnHeight, 1, height - lastHeight, drawLength, lastColor, color);
			lastDrawnHeight = height;
		}

		lastHeight = height;
		lastColor = color;
	}

	const auto cBgTint = v2cISSE16(bgTint);
	while (lastDrawnHeight < kTargetResX)
		pDest[lastDrawnHeight++] = cBgTint;
}

static void vball_precalc()
{
	const float radius = clampf(1.f, kMaxBallRadius, Rocket::getf(trackBallRadius));
	const float fRayLength = clampf(1.f, kMaxRayLength, Rocket::getf(trackBallRayLength)); // FIXME: use 'geti()'
	s_curRayLength = unsigned(fRayLength);

	// heights along ray wrap around half a circle
	const float angStep = kPI/(s_curRayLength-1);
	for (unsigned int iAngle = 0; iAngle < s_curRayLength; ++iAngle)
	{
		const float angle = angStep*iAngle;
		const float scale = radius*sinf(angle);
		s_heightProj[iAngle] = (unsigned) scale;
	}
}

// expected sizes:
// - maps: 512x512
static void vball(uint32_t *pDest, float time)
{
	// precalc. projection map (FIXME: it's just a multiplication and a sine, can't we move this to the ray function already?)
	vball_precalc();

	// select if has beams
	void (*vball_ray_fn)(uint32_t *, int, int, int, int, __m128i);
	vball_ray_fn = Rocket::geti(trackBallHasBeams) != 0 ? &vball_ray_beams : &vball_ray_no_beams;

	// move ray origin to fake hacky rotation (FIXME)
	const int fromX = ftofp24(512.f*cosf(time*0.25f) + 256.f);
	const int fromY = ftofp24(512.f*sinf(time*0.25f) + 256.f);

	// FOV (full circle)
	constexpr float fovAngle = kPI*2.f;
	constexpr float delta = fovAngle/kTargetResY;
//	float curAngle = 0.f;

	// background tint (FIXME: parametrize / do away with)
	const __m128i bgTint = c2vISSE16(0x1c075c);

	// cast rays
	#pragma omp parallel for schedule(static)
	for (int iRay = 0; iRay < kTargetResY; ++iRay)
	{
		float curAngle = iRay*delta;
		float dX, dY;
		voxel::calc_fandeltas(curAngle, dX, dY);
		vball_ray_fn(pDest + iRay*kTargetResX, fromX, fromY, ftofp24(dX), ftofp24(dY), bgTint);
//		curAngle += delta;
	}
}

// -- composition --

const char *kHeightMapPaths[2] =
{
	"assets/ball/hmap_4.jpg", // ball
	"assets/ball/hmap_1.jpg"  // spikey thing
};

bool Ball_Create()
{
	// load height maps
	for (int iMap = 0; iMap < 2; ++iMap)
	{
		s_pHeightMap[iMap] = Image_Load8(kHeightMapPaths[iMap]);
		if (nullptr == s_pHeightMap[iMap])
			return false;
	}
	
	// load color map
	s_pColorMap = Image_Load32("assets/ball/colormap.jpg");
	if (s_pColorMap == NULL)
		return false;

	// load beam map
	s_pBeamMap = Image_Load32("assets/ball/beammap.jpg");
	if (s_pBeamMap == NULL)
		return false;

	// load env. map
	s_pEnvMap = Image_Load32("assets/ball/envmap1.jpg");
	if (s_pEnvMap == NULL)
		return false;

	s_heightMapMix = static_cast<uint8_t*>(mallocAligned(512*512*sizeof(uint8_t), kCacheLine));

	// initialize sync. track(s)
	trackBallBlur = Rocket::AddTrack("ballBlur");
	trackBallRadius = Rocket::AddTrack("ballRadius");
	trackBallRayLength = Rocket::AddTrack("ballRayLength");
	trackBallSpikes = Rocket::AddTrack("ballSpikes");
	trackBallHasBeams = Rocket::AddTrack("ballHasBeams");

	return true;
}

void Ball_Destroy()
{
	freeAligned(s_heightMapMix);
}

void Ball_Draw(uint32_t *pDest, float time, float delta)
{
	// blend between map #0 (sphere-ish) and #1 (spikes)
	memcpy_fast(s_heightMapMix, s_pHeightMap[0], 512*512);
	const uint8_t spikes = (uint8_t) clampi(0, 255, Rocket::geti(trackBallSpikes));
	if (spikes != 0)
		Mix32(reinterpret_cast<uint32_t *>(s_heightMapMix), reinterpret_cast<uint32_t*>(s_pHeightMap[1]), 512*512/4 /* function processes 4 8-bit components at a time */, spikes);

	// render unwrapped ball
	vball(g_renderTarget, time);

	// polar blit
	Polar_Blit(g_renderTarget, pDest, false);

	memset32(pDest, 0xffffff, kResX*kResY);
	BlitSrc32(pDest + ((kResX-800)/2) + ((kResY-600)/2)*kResX, g_pNytrikMexico, kResX, 800, 600);

	// blur (optional)
	float blur = Rocket::getf(trackBallBlur);
	if (blur >= 1.f && blur <= 100.f)
	{
		blur *= kBoxBlurScale;
		memcpy(g_renderTarget, pDest, kOutputBytes);
		HorizontalBoxBlur32(pDest, g_renderTarget, kResX, kResY, blur);
	}

#if 0
	// debug blit: unwrapped
	const uint32_t *pSrc = g_renderTarget;
	for (unsigned int iY = 0; iY < kResY; ++iY)
	{
		memcpy(pDest, pSrc, kTargetResX*4);
		pSrc += kTargetResX;
		pDest += kResX;
	}
#endif
}
