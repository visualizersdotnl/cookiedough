
// cookiedough -- voxel tunnel

/*
	- for future fixes et cetera, see landscape.cpp, as the innerloop is much the same
*/

#include "main.h"
// #include "landscape.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
#include "polar.h"
#include "boxblur.h"
#include "voxel-shared.h"

static uint8_t *s_pHeightMap = NULL;
static uint32_t *s_pColorMap = NULL;
static uint32_t *s_pFogGradient = NULL;

static __m128i s_fogGradientUnp[256];

// -- voxel renderer --

// adjust to map (FIXME: parametrize)
const int kMapViewHeight = 110;
const int kMapTilt = 140;
const int kMapScale = 220;

// adjust to map resolution (1024x1024)
const unsigned int kMapAnd = 1023;                                         
const unsigned int kMapShift = 10;

// max. depth
const unsigned int kRayLength = 512; // 256 -- used for fog table!

static void tscape_ray(uint32_t *pDest, int curX, int curY, int dX, int dY)
{
	int lastHeight = kResX;
	int lastDrawnHeight = kResX;

	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE(s_pColorMap[U|V]);
	
	for (unsigned int iStep = 0; iStep < kRayLength; ++iStep)
	{
		// advance!
		curX += dX;
		curY += dY;

		// prepare UVs
		unsigned int U0, V0, U1, V1, fracU, fracV;
		bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);

		// fetch height & color
		const unsigned int mapHeight = bsamp8(s_pHeightMap, U0, V0, U1, V1, fracU, fracV);
		 __m128i color = bsamp32(s_pColorMap, U0, V0, U1, V1, fracU, fracV);

		// apply fog (modulate)
//		color = _mm_mullo_epi16(color, s_fogGradientUnp[iStep>>1]);
//		color = _mm_srli_epi16(color, 8);

		// apply fog (additive, no clamp: can overflow)
		color = _mm_adds_epu16(color, s_fogGradientUnp[iStep>>1]);

		// FIXME: now this is just a little convoluted :)
		int height = 256-mapHeight;		
		height -= kMapViewHeight;
		height <<= 8;
		height /= iStep+1; // FIXME
		height *= kMapScale;
		height >>= 8;
		height += kMapTilt;

		VIZ_ASSERT(height >= 0);

		// voxel visible?
		if (height < lastDrawnHeight)
		{
			// draw span (horizontal)
			const unsigned int drawLength = lastDrawnHeight - height;
			cspanISSE(pDest, 1, lastHeight - height, drawLength, color, lastColor);
			lastDrawnHeight = height;
			pDest += drawLength;
		}

		// comment for non-clipped span draw
		lastHeight = height;
		lastColor = color;
	}
}

// expected sizes:
// - maps: 1024x1024
static void tscape(uint32_t *pDest, float time)
{
	float mapX = 0.f; 
	const float mapStepX = 1024.f/(kTargetResY-1.f); // tile (for blit)

	for (unsigned int iRay = 0; iRay < kTargetResY; ++iRay)
	{
		const float fromX = mapX + time*66.f;
		const float fromY = 512.f + time*214.f;

		float dX, dY;
		dX = 0.5f;
		dY = 1.f; // FIXME: parametrize

		tscape_ray(pDest, ftof24(fromX), ftof24(fromY), ftof24(dX), ftof24(dY));
		pDest += kTargetResX;

		mapX += mapStepX;
	}

	return;
}

// -- composition --

bool Tunnelscape_Create()
{
	// load maps
	s_pHeightMap = Image_Load8("assets/scape/maps/D15.png");
	s_pColorMap = Image_Load32("assets/scape/maps/C15.png");
	if (s_pHeightMap == NULL || s_pColorMap == NULL)
		return false;

	// load fog gradient (8-bit LUT)
	s_pFogGradient = Image_Load32("assets/scape/foggradient.jpg");
	if (s_pFogGradient == NULL)
		return false;
		
	// unpack fog gradient pixels
	for (int iPixel = 0; iPixel < 256; ++iPixel)
		s_fogGradientUnp[iPixel] = c2vISSE(s_pFogGradient[iPixel]);

	return true;
}

void Tunnelscape_Destroy()
{
	Image_Free(s_pHeightMap);
	Image_Free(s_pColorMap);
	Image_Free(s_pFogGradient);
}

void Tunnelscape_Draw(uint32_t *pDest, float time, float delta)
{
	memset32(g_renderTarget, s_pFogGradient[255], kTargetResX*kTargetResY);
	tscape(g_renderTarget, time);

	// radial blur
	HorizontalBoxBlur32(g_renderTarget, g_renderTarget, kTargetResX, kTargetResY, 0.00314f);

	// polar blit
	Polar_Blit(g_renderTarget, pDest, true);
}
