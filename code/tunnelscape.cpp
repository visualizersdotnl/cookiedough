
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
#include "rocket.h"

// Sync.
SyncTrack trackStarsDirX, trackStarsDirY;

static uint8_t *s_pHeightMap = NULL;
static uint32_t *s_pColorMap = NULL;
static uint32_t *s_pFogGradient = NULL;

static __m128i s_fogGradientUnp[256];

// -- voxel renderer --

// adjust to map (FIXME: parametrize)
constexpr float kMapViewLenScale = 0.314*0.628f;
constexpr int kMapViewHeight = 110;
constexpr int kMapTilt = 140;
constexpr int kMapScale = 120;

// adjust to map resolution (1024x1024)
constexpr unsigned kMapSize = 1024;
constexpr unsigned kMapAnd = kMapSize-1;                                          
constexpr unsigned kMapShift = 10;

// max. depth
const unsigned int kRayLength = 512; // 256 -- used for fog table!

static void tscape_ray(uint32_t *pDest, int curX, int curY, int dX, int dY)
{
	int lastHeight = kResX;
	int lastDrawnHeight = kResX;

	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE16(s_pColorMap[U|V]);
	
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
		 __m128i color = bsamp32_16(s_pColorMap, U0, V0, U1, V1, fracU, fracV);

		// apply fog (modulate)
//		color = _mm_mullo_epi16(color, s_fogGradientUnp[iStep>>1]);
//		color = _mm_srli_epi16(color, 8);

		// apply fog (additive/subtractive, no clamp: can overflow)
//		color = _mm_adds_epu16(color, s_fogGradientUnp[iStep>>1]);
		color = _mm_subs_epu16(color, s_fogGradientUnp[iStep>>1]);

		// FIXME: now this is just a little convoluted :)
		int height = 256-mapHeight;		
		height -= kMapViewHeight;
		height <<= 8;
		height /= kMapViewLenScale; // FIXME: no idea why this works, but it does
		height /= iStep+1;          //
		height *= kMapScale;
		height >>= 8;
		height += kMapTilt;

		VIZ_ASSERT(height >= 0);

		// voxel visible?
		if (height < lastDrawnHeight)
		{
			// draw span (horizontal)
			const unsigned int drawLength = lastDrawnHeight - height;
			cspanISSE16(pDest, 1, lastHeight - height, drawLength, color, lastColor);
			lastDrawnHeight = height;
			pDest += drawLength;
		}

		lastHeight = height;
		lastColor = color;
	}
}

// expected sizes:
// - maps: 1024x1024
static void tscape(uint32_t *pDest, float time)
{
//	float mapX = 0.f; 
	constexpr float mapStepX = 1024.f/(kTargetResY-1.f); // tile (for blit)

	const float syncDirX = Rocket::getf(trackStarsDirX);
	const float syncDirY = Rocket::getf(trackStarsDirY);

	const float fromY = 512.f + syncDirY*time*214.f;

	const int dX = ftofp24(0.5f);
	const int dY = ftofp24(1.f);

	#pragma omp parallel for schedule(static)
	for (int iRay = 0; iRay < kTargetResY; ++iRay)
	{
		const float mapX = iRay*mapStepX;
		const float fromX = mapX  + syncDirX*time*66.f;

		tscape_ray(pDest + iRay*kTargetResX, ftofp24(fromX), ftofp24(fromY), dX, dY);

//		pDest += kTargetResX;
//		mapX += mapStepX;
	}

	return;
}

// -- composition --

bool Tunnelscape_Create()
{
	// load maps
	s_pHeightMap = Image_Load8("assets/scape/maps/D7.png");
	s_pColorMap = Image_Load32("assets/scape/maps/C7W.png");
	if (s_pHeightMap == NULL || s_pColorMap == NULL)
		return false;

	// load fog gradient (8-bit LUT)
	s_pFogGradient = Image_Load32("assets/scape/foggradient.jpg");
	if (s_pFogGradient == NULL)
		return false;
		
	// unpack fog gradient pixels
	for (int iPixel = 0; iPixel < 256; ++iPixel)
		s_fogGradientUnp[iPixel] = c2vISSE16(s_pFogGradient[iPixel]);

	trackStarsDirX = Rocket::AddTrack("starsTunnelDirX");
	trackStarsDirY = Rocket::AddTrack("starsTunnelDirY");

	return true;
}

void Tunnelscape_Destroy()
{
}

void Tunnelscape_Draw(uint32_t *pDest, float time, float delta)
{
	memset32(g_renderTarget, s_pFogGradient[0], kTargetResX*kTargetResY);
	tscape(g_renderTarget, time);

	// radial blur
	HorizontalBoxBlur32(g_renderTarget, g_renderTarget, kTargetResX, kTargetResY, 0.00314f);

	// polar blit
	Polar_Blit(g_renderTarget, pDest, true);
}
