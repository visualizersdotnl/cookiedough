
// cookiedough -- voxel torus twister (640x480)

#include "main.h"
// #include "torus-twister.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
#include "boxblur.h"
#include "shared.h"
#include "polar.h"

static uint8_t *s_pHeightMap = NULL;
static uint32_t *s_pColorMap = NULL;
static uint32_t *s_pBeamMap = NULL;

// -- voxel renderer --

// adjust to map resolution
const unsigned int kMapAnd = 511;                                          
const unsigned int kMapShift = 9;

// max. depth
const unsigned int kRayLength = 256;

// height projection table
static unsigned int s_heightProj[kRayLength];

// max. radius (in pixels)
const float kCylRadius = 300.f;

// scale applied to each beam sample
const uint8_t kBeamMul = 255;

static void vtwister_ray(uint32_t *pDest, int curX, int curY, int dX)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE(s_pColorMap[U+V]);
	__m128i beamAccum = _mm_setzero_si128();

	const int direction = (dX < 0) ? -1 : 1;

	// render ray
	for (unsigned int iStep = 0; iStep < kRayLength; ++iStep)
	{
		// prepare UVs
		unsigned int U0, V0, U1, V1, fracU, fracV;
		bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);

		// fetch height & unpacked color
		const unsigned int mapHeight = bsamp8(s_pHeightMap, U0, V0, U1, V1, fracU, fracV);
		__m128i color = bsamp32(s_pColorMap, U0, V0, U1, V1, fracU, fracV);

		// fetch, accumulate & add beam (separate map)
		{
			const __m128i beam = bsamp32(s_pBeamMap, U0, V0, U1, V1, fracU, fracV);
//			const __m128i beam = c2vISSE(s_pBeamMap[U0+V0]);
//			beamAccum = _mm_adds_epu16(beamAccum, beam);
			
			// pre-multiply
//			__m128i beamMul = g_gradientUnp[kBeamMul];
//			beamAccum = _mm_adds_epu16(beamAccum, _mm_srli_epi16(_mm_mullo_epi16(beam, beamMul), 8));

			// clamps & add
//			beamAccum = vminISSE(beamAccum, g_gradientUnp[255]);
//			color = vminISSE(_mm_adds_epu16(color, beamAccum), g_gradientUnp[255]);

			// just add (overflow "trick")
			color = _mm_adds_epu16(color, beamAccum);
		}
		
		// project height
		const unsigned int height = mapHeight*s_heightProj[iStep] >> 8;

		// voxel visible?
		if (height > lastDrawnHeight)
		{
			// draw span
			const unsigned int drawLength = height - lastDrawnHeight;
			cspanISSE(pDest, direction, height - lastHeight, drawLength, lastColor, color);
			pDest += drawLength*direction;
			lastDrawnHeight = height;
		}

		lastHeight = height;
		lastColor = color;

		// advance!
		curX += dX;
	}

#if 0
	// beams (doesn't look very good)
	int remainder;
	if (direction > 0)
	{
		remainder = 319-lastDrawnHeight;
		if (remainder > 0)
			cspanISSE_noclip(pDest, direction, remainder, beamAccum, _mm_setzero_si128()); 
	}
	else
	{
		remainder = 320-lastDrawnHeight;
//		remainder -= 30;
		if (remainder > 0)
			cspanISSE_noclip(pDest, direction, remainder, beamAccum, _mm_setzero_si128());
	}
#endif
}

// expected sizes:
// - render target: 640x480
// - maps: 512x512
static void vtwister(uint32_t *pDest, float time)
{
	// cast parallel rays (FIXME, look at tunnelscape.cpp!)

	// must tile! (for polar blit)
	float mapY = 0.f; 
	const float mapStepY = 512.f/480.f; 

	for (unsigned int iRay = 0; iRay < 480; ++iRay)
	{
		const float shearAngle = (float) iRay * (2.f*kPI / 480.f);

		const int fromX = ftof24(256.f + 140.f*sinf(time*1.1f + shearAngle));
		const int fromY = ftof24(mapY + time*25.f);

		vtwister_ray(pDest + iRay*640 + 320, fromX, fromY,  256);
		vtwister_ray(pDest + iRay*640 + 319, fromX, fromY, -256);

		mapY += mapStepY;
	}
}

void vtwister_precalc()
{
	// heights along ray wrap around half a circle
	for (unsigned int iAngle = 0; iAngle < kRayLength; ++iAngle)
	{
		const float angle = kPI/kRayLength * iAngle;
		const float scale = kCylRadius*sinf(angle);
		s_heightProj[iAngle] = (unsigned) scale;
	}
}

// -- composition --

bool Twister_Create()
{
	vtwister_precalc();

	// load maps
	s_pHeightMap = Image_Load8("assets/twister/hmap_2.jpg");
	s_pColorMap = Image_Load32("assets/twister/colormap.jpg");
	s_pBeamMap = Image_Load32("assets/ball/beammap.jpg");
	if (s_pHeightMap == NULL || s_pColorMap == NULL || s_pBeamMap == NULL)
		return false;

	return true;
}

void Twister_Destroy()
{
	Image_Free(s_pHeightMap);
	Image_Free(s_pColorMap);
	Image_Free(s_pBeamMap);
}

void Twister_Draw(uint32_t *pDest, float time)
{
	// render twister
	memset32(g_renderTarget, 0x1f0053, 640*480);
	vtwister(g_renderTarget, time);

	// (radial) blur
	HorizontalBoxBlur32(g_renderTarget, g_renderTarget, 640, 480, 0.04f);

	// polar blit
	Polar_Blit(g_renderTarget, pDest);
}
