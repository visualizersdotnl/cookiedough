
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
SyncTrack trackBallRayLength; // FIXME: implement!

// --------------------
	
// -- voxel renderer --

// my notes about the current situation:
// - fix environment mapping
// - shape hmap_1.jpg looks *great* without environment mapping, and with very subdued beams, what I'm thinking is to use the blur and positioning
//   to synchronize this as an effect
// - shape hmap_4.jpg looks good with proper beams on, possibly with an environment map; could do with some art
//   + doesn't work all that well on a dark background?
//   + I'm unsure what Orange themselves are doing, they have some kind of algorithm that's different from mine
// - start parametrizing!
// - ...

// #define NO_BEAMS
// #define NO_BEAM_EXTRUSION
#define STATIC_SHAPE 1
#define NO_ENV_MAP

// adjust to map resolution
constexpr unsigned kMapSize = 512;
constexpr unsigned kMapAnd = kMapSize-1;                                          
constexpr unsigned kMapShift = 9;

// max. depth
constexpr unsigned kRayLength = 512;

// height projection table
static unsigned int s_heightProj[kRayLength];

// max. radius (in pixels)
constexpr float kBallRadius = 800.f;

// scale applied to each beam sample
constexpr auto kBeamMul = 4;

// how much (indexing gradient) to darken the beam while it's extruded
constexpr auto kBeamExtMul = 3;

static void vball_ray(uint32_t *pDest, int curX, int curY, int dX, int dY, __m128i bgTint)
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

	for (unsigned int iStep = 0; iStep < kRayLength; ++iStep)
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

#if !defined(NO_BEAMS)
		// and beam (extrusion) color (while the prepared UVs are still intact)
		const __m128i beam = bsamp32_16(s_pBeamMap, U0, V0, U1, V1, fracU, fracV);

		// accumulate & add beam (separate map)
		beamAccum = _mm_adds_epu16(beamAccum, _mm_srli_epi16(_mm_mullo_epi16(beam, beamMul), 8));
		color = _mm_adds_epu16(color, beamAccum);
#endif

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

#if defined(NO_BEAMS) || defined(NO_BEAM_EXTRUSION)

	while (lastDrawnHeight < kTargetResX)
		pDest[lastDrawnHeight++] = 0;

#else

	// conservative beam glow (FIXME: remove?)
//
//	while (lastDrawnHeight < kTargetResX)
//	{
//		pDest[lastDrawnHeight++] = v2cISSE16(beamAccum);
//		beamAccum = _mm_srli_epi16(_mm_mullo_epi16(beamAccum, beamExtMul), 8);
//	}

	// beam fade out
	const unsigned int remainder = kTargetResX-lastDrawnHeight;
	cspanISSE16_noclip(pDest + lastDrawnHeight, 1, remainder, beamAccum, bgTint); 

#endif
}

// expected sizes:
// - maps: 512x512
static void vball(uint32_t *pDest, float time)
{
	// move ray origin to fake hacky rotation (FIXME)
	const int fromX = ftofp24(512.f*cosf(time*0.25f) + 256.f);
	const int fromY = ftofp24(512.f*sinf(time*0.25f) + 256.f);

	// FOV (full circle)
	constexpr float fovAngle = kPI*2.f;
	constexpr float delta = fovAngle/kTargetResY;
//	float curAngle = 0.f;

	const __m128i bgTint = c2vISSE16(0x3fa2e1);

	// cast rays
	#pragma omp parallel for schedule(static)
	for (int iRay = 0; iRay < kTargetResY; ++iRay)
	{
		float curAngle = iRay*delta;
		float dX, dY;
		voxel::calc_fandeltas(curAngle, dX, dY);
		vball_ray(pDest + iRay*kTargetResX, fromX, fromY, ftofp24(dX), ftofp24(dY), bgTint);
//		curAngle += delta;
	}
}

static void vball_precalc()
{
	// heights along ray wrap around half a circle
	for (unsigned int iAngle = 0; iAngle < kRayLength; ++iAngle)
	{
		const float angle = kPI/(kRayLength-1) * iAngle;
		const float scale = kBallRadius*sinf(angle);
		s_heightProj[iAngle] = (unsigned) scale;
	}
}

// -- composition --

const char *kHeightMapPaths[2] =
{
	"assets/ball/hmap_1.jpg",
	"assets/ball/hmap_4.jpg"
};

bool Ball_Create()
{
	vball_precalc();

	// load height maps
	for (int iMap = 0; iMap < 2; ++iMap)
	{
		s_pHeightMap[iMap] = Image_Load8(kHeightMapPaths[iMap]);
		if (s_pHeightMap[iMap] == NULL)
		{
			return false;
		}
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
	trackBallRayLength = Rocket::AddTrack("ballRayLength");

	return true;
}

void Ball_Destroy()
{
	freeAligned(s_heightMapMix);
}

void Ball_Draw(uint32_t *pDest, float time, float delta)
{
	// static height map
	memcpy_fast(s_heightMapMix, s_pHeightMap[STATIC_SHAPE], 512*512);

	// render unwrapped ball
	vball(g_renderTarget, time);

	// polar blit
	Polar_Blit(g_renderTarget, pDest, false);

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

#if 0
	// debug blit: map
	const uint8_t *pSrc = s_heightMapMix;
	for (unsigned int iY = 0; iY < kResY; ++iY)
	{
		for (int iX = 0; iX < 512; ++iX)
			pDest[iX] = pSrc[iX] * 0x010101;

		pSrc += 512;
		pDest += kResX;
	}
#endif
}
