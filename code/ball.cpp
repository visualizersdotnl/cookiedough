
// cookiedough -- voxel ball (2-pass approach)

/*
	- FIXMEs
*/

#include "main.h"
// #include "ball.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
#include "boxblur.h"
#include "polar.h"
#include "voxel-shared.h"

static uint8_t *s_pHeightMap[5] = { nullptr };
static uint32_t *s_pColorMap = nullptr;
static uint32_t *s_pBeamMap = nullptr;
static uint8_t *s_heightMapMix = nullptr;

// -- voxel renderer --

// adjust to map resolution
const unsigned int kMapAnd = 511;                                          
const unsigned int kMapShift = 9;

// max. depth
const unsigned int kRayLength = 256;

// height projection table
static unsigned int s_heightProj[kRayLength];

// max. radius (in pixels)
const float kBallRadius = 510.f;

// scale applied to each beam sample
const uint8_t kBeamMul = 128;

static void vball_ray(uint32_t *pDest, int curX, int curY, int dX, int dY)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE(s_pColorMap[U+V]);
	__m128i beamAccum = _mm_setzero_si128();

	for (unsigned int iStep = 0; iStep < kRayLength; ++iStep)
	{
		// advance!
		curX += dX;
		curY += dY;

		// prepare UVs
		unsigned int U0, V0, U1, V1, fracU, fracV;
		bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
//		unsigned int U0 = curX >> 8 & kMapAnd, V0 = (curY >> 8 & kMapAnd) << kMapShift;

		// fetch height & unpacked color
		const unsigned int mapHeight = bsamp8(s_heightMapMix, U0, V0, U1, V1, fracU, fracV);
		__m128i color = bsamp32(s_pColorMap, U0, V0, U1, V1, fracU, fracV);
//		const unsigned int mapHeight = s_heightMapMix[U0+V0];
//		__m128i color = c2vISSE(s_pColorMap[U0+V0]);

		// fetch, accumulate & add beam (separate map)
		{
			const __m128i beam = bsamp32(s_pBeamMap, U0, V0, U1, V1, fracU, fracV);
//			const __m128i beam = c2vISSE(s_pBeamMap[U0+V0]);
			beamAccum = _mm_adds_epu16(beamAccum, beam);
			
			// pre-multiply
			__m128i beamMul = g_gradientUnp[kBeamMul];
			beamAccum = _mm_adds_epu16(beamAccum, _mm_srli_epi16(_mm_mullo_epi16(beam, beamMul), 8));

			// clamps & add
//			beamAccum = vminISSE(beamAccum, g_gradientUnp[255]);
//			color = vminISSE(_mm_adds_epu16(color, beamAccum), g_gradientUnp[255]);

			// just add (overflow "trick")
//			color = _mm_adds_epu16(color, beamAccum);
		}

		// accumulate & add beam (alpha channel)
//		const __m128i alpha = _mm_shufflelo_epi16(color, 0xff));
		// ...

		// project height
		const unsigned int height = mapHeight*s_heightProj[iStep] >> 8;

		// voxel visible?
		if (height > lastDrawnHeight)
		{
			const unsigned int drawLength = height - lastDrawnHeight;

			// draw span (clipped)
			cspanISSE(pDest + lastDrawnHeight, 1, height - lastHeight, drawLength, lastColor, color);
			lastDrawnHeight = height;

			// draw span
//			cspanISSE_noclip(pDest + lastDrawnHeight, 1, drawLength, lastColor, color);
//			lastDrawnHeight = height;
//			lastColor = color;
		}

		// comment for non-clipped span draw
		lastHeight = height;
		lastColor = color;
	}

#if 1
	// opaque
	for (int iPixel = lastDrawnHeight; iPixel < kTargetResX; ++iPixel)
		pDest[iPixel] = 0;
#endif

#if 0
	// beam-only (debug)
	const unsigned int remainder = kTargetResX;
	cspanISSE_noclip(pDest, 1, remainder, beamAccum, _mm_setzero_si128()); 
#endif

#if 0
	// beams (fade out, also works with overflowing beam)
	const unsigned int remainder = kTargetResX-lastDrawnHeight;
	cspanISSE_noclip(pDest + lastDrawnHeight, 1, remainder, beamAccum, _mm_setzero_si128()); 
#endif

#if 0
	// beams (full brightness)
	const uint32_t color = v2cISSE(beamAccum);
	for (int iPixel = lastDrawnHeight; iPixel < kTargetResX; ++iPixel)
		pDest[iPixel] = color;
#endif

#if	0
	// glow (only looks good with unclamped overflowing beam!)
 	const __m128i beamSub = g_gradientUnp[4];
	for (int iPixel = lastDrawnHeight; iPixel < kTargetResX; ++iPixel)
	{
		pDest[iPixel] = v2cISSE(beamAccum);
		beamAccum = _mm_subs_epu16(beamAccum, beamSub);
	}
#endif
}

// expected sizes:
// - maps: 512x512
static void vball(uint32_t *pDest, float time)
{
	// move ray origin to fake hacky rotation (FIXME)
	const int fromX = ftof24(256.f*sinf(time*0.28f) + 256.f);
	const int fromY = ftof24(256.f*cosf(time*0.25f) + 256.f);

	// FOV (full circle)
	const float fovAngle = kPI*2.f;
	const float delta = fovAngle/kTargetResY;
	float curAngle = 0.f;

	// cast rays
	for (unsigned int iRay = 0; iRay < kTargetResY; ++iRay)
	{
		float dX, dY;
		calc_fandeltas(curAngle, dX, dY);
		vball_ray(pDest + iRay*kTargetResX, fromX, fromY, ftof24(dX), ftof24(dY));
		curAngle += delta;
	}
}

static void vball_precalc()
{
	// heights along ray wrap around half a circle
	for (unsigned int iAngle = 0; iAngle < kRayLength; ++iAngle)
	{
		const float angle = kPI/kRayLength * iAngle;
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
//	s_pBeamMap = Image_Load32("assets/unused/ibiza_rip/x11.jpg");
	s_pBeamMap = Image_Load32("assets/ball/beammap.jpg");
	if (s_pBeamMap == NULL)
		return false;

	s_heightMapMix = static_cast<uint8_t*>(mallocAligned(512*512*sizeof(uint8_t), kCacheLine));

	return true;
}

void Ball_Destroy()
{
	for (int iMap = 0; iMap < 5; ++iMap) 
		Image_Free(s_pHeightMap[iMap]);
	
	Image_Free(s_pColorMap);
	Image_Free(s_pBeamMap);

	freeAligned(s_heightMapMix);
}

void Ball_Draw(uint32_t *pDest, float time, float delta)
{
	// mix height map (FIXME)
	float mapMix = fmodf(time*2.f, 128.f) / 16.f;
	if (mapMix > 4.f) mapMix = 4.f - (mapMix - 4.f);
	const float mapMixHi = ceilf(mapMix), mapMixLo = floorf(mapMix);
	const uint8_t mapMixAlpha = (uint8_t) ((mapMix-mapMixLo)*255.f);
	uint8_t *pMap1, *pMap2;
	VIZ_ASSERT(mapMixHi < 5);
	pMap1 = s_pHeightMap[(int) mapMixLo];
	pMap2 = s_pHeightMap[(int) mapMixHi];
	memcpy_fast(s_heightMapMix, pMap1, 512*512);
	Mix32(reinterpret_cast<uint32_t *>(s_heightMapMix), reinterpret_cast<uint32_t *>(pMap2), 512*512/4, mapMixAlpha);	

	// static height map
	// memcpy_fast(s_heightMapMix, s_pHeightMap[2], 512*512);

	// render unwrapped ball
	vball(g_renderTarget, time);

	// radial blur
	// HorizontalBoxBlur32(g_renderTarget, g_renderTarget, kTargetResX, kTargetResY, 0.01628f);

	// polar blit
	Polar_Blit(g_renderTarget, pDest, false);

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
