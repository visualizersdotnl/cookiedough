
// cookiedough -- voxel landscape

/*
	- more fixed point (+ LUT)
	- fix proper controller input & airship HUD for fun, then move generic code out
*/

#include "main.h"
// #include "landscape.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
#include "polar.h"
#include "boxblur.h"
#include "voxel-shared.h"
#include "gamepad.h"

static uint8_t *s_pHeightMap = nullptr;
static uint32_t *s_pColorMap = nullptr;
static uint32_t *s_pHUD = nullptr;
static uint32_t *s_pFogGradient = nullptr;

static __m128i s_fogGradientUnp[256];

// -- voxel renderer --

// adjust to map (FIXME: parametrize)
const int kMapViewHeight = 80;
const int kMapTilt = 60;
const int kMapScale = 200;

// adjust to map resolution
const unsigned int kMapAnd = 1023;                                         
const unsigned int kMapShift = 10;

// max. depth
const unsigned int kRayLength = 256;

// sample height (filtered)
__forceinline unsigned int vscape_shf(int U, int V)
{
	unsigned int U0, V0, U1, V1, fracU, fracV;
	bsamp_prepUVs(U, V, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
	return bsamp8(s_pHeightMap, U0, V0, U1, V1, fracU, fracV);
}

static void vscape_ray(uint32_t *pDest, int curX, int curY, int dX, int dY, float fishMul)
{
	int lastHeight = kResY;
	int lastDrawnHeight = kResY;

	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE(s_pColorMap[U|V]);

	// fishMul = fabsf(fishMul);
	const int fpFishMul = ftof24(fabsf(fishMul));
	
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
//		color = _mm_mullo_epi16(color, s_fogGradientUnp[iStep]);
//		color = _mm_srli_epi16(color, 8);

		// apply fog (additive/subtractive, no clamp: can overflow)
//		color = _mm_adds_epu16(color, s_fogGradientUnp[iStep]);
		color = _mm_subs_epu16(color, s_fogGradientUnp[iStep]);

		int height = 256-mapHeight;		
		height -= kMapViewHeight;
		height <<= 16;
		height /= fpFishMul*(iStep+1); // FIXME
		height *= kMapScale;
		height >>= 8;
		height += kMapTilt;

		VIZ_ASSERT(height >= 0);

		// voxel visible?
		if (height < lastDrawnHeight)
		{
			// draw span (vertical)
			const unsigned int drawLength = lastDrawnHeight - height;
			cspanISSE(pDest + height*kResX, kResX, lastHeight - height, drawLength, color, lastColor);
			lastDrawnHeight = height;
		}

		// comment for non-clipped span draw
		lastHeight = height;
		lastColor = color;
	}
}

// expected sizes:
// - maps: 1024x1024
// FIXME: make this fixed point if at some point you care enough
static void vscape(uint32_t *pDest, float time)
{
	// to do right here:
	// - gamepad poll should return a simple structure
	// - move lowpass to util.h
	// - scale leftY so speed remains constant while navigating (use hypothenuse)
	// - add some banking

	float leftX, leftY, rightX, rightY;
	Gamepad_Update(leftX, leftY, rightX, rightY);

	static float lowpass = 0.f;
	lowpass = 0.125f*leftX + 0.875f*lowpass;

	const float viewAngle = -lowpass*0.25f*kPI;
	const float angCos = cosf(viewAngle);
	const float angSin = sinf(viewAngle);

	static float origX = 512.f;
	static float origY = 512.f;

	origX += 0.f;
	origY -= leftY;

	// FIXME: formalize as parameter
	float rayY = 370.f;

	for (unsigned int iRay = 0; iRay < kResX; ++iRay)
	{
		const float rayX = (kPI*0.45f)*(iRay - kResX*0.5f);

		const float rotX = angCos*rayX - angSin*rayY;
		const float rotY = angSin*rayX + angCos*rayY;

		const float X1 = origX;
		const float Y1 = origY;
		const float X2 = origX+rotX;
		const float Y2 = origY+rotY;

		// normalize step value
		float dX = X2-X1;
		float dY = Y2-Y1;
		normalize_vdeltas(dX, dY);

		// counteract fisheye effect
		float fishMul = rayY / sqrtf(rayX*rayX + rayY*rayY);
		
		vscape_ray(pDest+iRay, ftof24(X1), ftof24(Y1), ftof24(dX), ftof24(dY), fishMul);

	}
}

// -- composition --

bool Landscape_Create()
{
	VIZ_ASSERT(kResX == 800 && kResY == 600);

	// load maps
	s_pHeightMap = Image_Load8("assets/scape/maps/D24.png");
	s_pColorMap = Image_Load32("assets/scape/maps/C24W.png");
	s_pHUD = Image_Load32("assets/scape/aircraft_hud.jpg");
	if (nullptr == s_pHeightMap || nullptr == s_pColorMap|| nullptr == s_pHUD)
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

void Landscape_Destroy()
{
	Image_Free(s_pHeightMap);
	Image_Free(s_pColorMap);
	Image_Free(s_pHUD);
	Image_Free(s_pFogGradient);
}

void Landscape_Draw(uint32_t *pDest, float time)
{
	// render landscape (FIXME: take input)
	memset32(pDest, s_pFogGradient[0], kResX*kResY);
	vscape(pDest, time);

	// overlay HUD
	Add32(pDest, s_pHUD, kResX*kResY);
}

