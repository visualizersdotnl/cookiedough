
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

static uint8_t *s_pHeightMap[5] = { nullptr };
static uint32_t *s_pColorMap = nullptr;
static uint32_t *s_pBeamMap = nullptr;
static uint32_t *s_pEnvMap = nullptr;
static uint8_t *s_heightMapMix = nullptr;

// --- Sync. tracks ---

SyncTrack trackBallBlur;

// --------------------
	
// -- voxel renderer --

// #define NO_BEAMS
#define STATIC_BALL_SHAPE

// adjust to map resolution
const unsigned kMapSize = 512;
constexpr unsigned kMapAnd = kMapSize-1;                                          
const unsigned kMapShift = 9;

// max. depth
const unsigned int kRayLength = 400;

// height projection table
static unsigned int s_heightProj[kRayLength];

// max. radius (in pixels)
const float kBallRadius = 850.f;

// scale applied to each beam sample
const uint8_t kBeamMul = 4;

static void vball_ray(uint32_t *pDest, int curX, int curY, int dX, int dY)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	const unsigned int U = curX >> 8 & kMapAnd, V = (-curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE16(s_pColorMap[U+V]);

	__m128i beamAccum = _mm_setzero_si128();
	__m128i beamMul = g_gradientUnp[kBeamMul];

	int envU = (kMapSize>>1)<<8;
	int envV = envU;

	for (unsigned int iStep = 0; iStep < kRayLength; ++iStep)
	{
		// I had the mapping the wrong way around (or I am adjusting for something odd elsewhere, but it works for now (FIXME))
		curX -= dX;
		curY -= dY;
		envU -= dX;
		envV -= dY;

		// prepare UVs
		unsigned int U0, V0, U1, V1, fracU, fracV;
		bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);

		// fetch height & unpacked color
		const unsigned int mapHeight = bsamp8(s_heightMapMix, U0, V0, U1, V1, fracU, fracV);
		__m128i color = bsamp32_16(s_pColorMap, U0, V0, U1, V1, fracU, fracV);

#if !defined(NO_BEAMS)
		// and beam (extrusion) color (while the prepared UVs are still intact)
		const __m128i beam = bsamp32_16(s_pBeamMap, U0, V0, U1, V1, fracU, fracV);
#endif

		// sample env. map
		// const unsigned int U = envU >> 8 & kMapAnd, V = (envV >> 8 & kMapAnd) << kMapShift;
		bsamp_prepUVs(envU, envV, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
		const __m128i additive = bsamp32_16(s_pEnvMap, U0, V0, U1, V1, fracU, fracV);
		color = _mm_adds_epu16(color, additive);

#if !defined(NO_BEAMS)

		// accumulate & add beam (separate map)
		beamAccum = _mm_adds_epu16(beamAccum, _mm_srli_epi16(_mm_mullo_epi16(beam, beamMul), 8));
		color = _mm_adds_epu16(color, beamAccum);

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

#if defined(NO_BEAMS)

	while (lastDrawnHeight < kTargetResX)
		pDest[lastDrawnHeight++] = 0;

#else

	// conservative beam glow
 	const __m128i beamSub = g_gradientUnp[3];
	while (lastDrawnHeight < kTargetResX)
	{
		pDest[lastDrawnHeight++] = v2cISSE16(beamAccum);
		beamAccum = _mm_subs_epu16(beamAccum, beamSub);
	}

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

	// cast rays
	#pragma omp parallel for schedule(static)
	for (int iRay = 0; iRay < kTargetResY; ++iRay)
	{
		float curAngle = iRay*delta;
		float dX, dY;
		voxel::calc_fandeltas(curAngle, dX, dY);
		vball_ray(pDest + iRay*kTargetResX, fromX, fromY, ftofp24(dX), ftofp24(dY));
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

const char *kHeightMapPaths[5] =
{
	"assets/ball/hmap_1.jpg",
	"assets/ball/hmap_2.jpg",
	"assets/ball/hmap_3.jpg",
	"assets/ball/hmap_4.jpg",
	"assets/ball/hmap_5.jpg"
};

bool Ball_Create()
{
	vball_precalc();

	// load height maps
	for (int iMap = 0; iMap < 5; ++iMap)
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
	s_pEnvMap = Image_Load32("assets/ball/envmap2.jpg");
	if (s_pEnvMap == NULL)
		return false;

	s_heightMapMix = static_cast<uint8_t*>(mallocAligned(512*512*sizeof(uint8_t), kCacheLine));

	// initialize sync. track(s)
	trackBallBlur = Rocket::AddTrack("ballBlur");

	return true;
}

void Ball_Destroy()
{
	freeAligned(s_heightMapMix);
}

void Ball_Draw(uint32_t *pDest, float time, float delta)
{
#if !defined(STATIC_BALL_SHAPE)
	
	// FIXME: should I keep this at all? doesn't look all that great!
	float mapMix = fmodf(time*2.f, 128.f) / 16.f;
	if (mapMix > 4.f) mapMix = 4.f - (mapMix - 4.f);
	mapMix = smoothstepf(0.f, 4.f, mapMix/4.f);
	const float mapMixHi = ceilf(mapMix), mapMixLo = floorf(mapMix);
	const uint8_t mapMixAlpha = (uint8_t) ((mapMix-mapMixLo)*255.f);
	uint8_t *pMap1, *pMap2;
	VIZ_ASSERT(mapMixHi < 5);
	pMap1 = s_pHeightMap[(int) mapMixLo];
	pMap2 = s_pHeightMap[(int) mapMixHi];
	memcpy_fast(s_heightMapMix, pMap1, 512*512);
	Mix32(reinterpret_cast<uint32_t *>(s_heightMapMix), reinterpret_cast<uint32_t *>(pMap2), 512*512/4, mapMixAlpha);	

#else

	// static height map
	memcpy_fast(s_heightMapMix, s_pHeightMap[3], 512*512);

#endif

	// render unwrapped ball
	vball(g_renderTarget, time);

	// polar blit
	Polar_Blit(g_renderTarget, pDest, false);

	// horiz. blur (optional)
	float blur = Rocket::getf(trackBallBlur);
	if (blur >= 1.f && blur <= 100.f)
	{
		blur *= kBoxBlurScale;
		HorizontalBoxBlur32(pDest, pDest, kResX, kResY, blur);
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
