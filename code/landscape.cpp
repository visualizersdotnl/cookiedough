
// cookiedough -- voxel landscape

/*
	- more fixed point (+ LUT)
	- fix proper controller input & airship HUD for fun: fix banking and ship rotation!
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

// adjust to map (FIXME: parametrize, document)
const float kMapViewLenScale = 0.314f;
const int kMapViewHeight = 60;
const int kMapTilt = 160;
const int kMapScale = 180;

// adjust to map resolution
const unsigned int kMapAnd = 1023;                                         
const unsigned int kMapShift = 10;

// max. depth
const unsigned int kRayLength = 512;

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
		color = _mm_subs_epu16(color, s_fogGradientUnp[iStep>>1]);

		int height = 255-mapHeight;		
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

static void vscape(uint32_t *pDest, float time, float delta)
{
	// grab gamepad input
	float leftX, leftY, rightX, rightY;
	Gamepad_Update(delta, leftX, leftY, rightX, rightY);

	// calc. view angle sine & cosine	
	static float viewAngle = 33.f;
	const float angCos = cosf(viewAngle);
	const float angSin = sinf(viewAngle);

	// strafe
	static float strafe = 0.f;
	strafe += rightX;
	float strafeX = angCos*strafe; // - angSin*0.f;
	float strafeY = angSin*strafe; // + angCos*0.f;

	// move
	static float move = 0.f;
	move -= leftY;
	float moveX = -angSin*move; // angCos*0.f - angSin*move;
	float moveY = angCos*move;  // angSin*0.f + angCos*move;

	// origin
	const float origX = 0.f;
	const float origY = 0.f;
	const float X1 = origX+strafeX+moveX;
	const float Y1 = origY+strafeY+moveY;
	const int fpX1 = ftof24(X1);
	const int fpY1 = ftof24(Y1);

	const float rayY = (kMapAnd+1)*kMapViewLenScale;

	// FIXME: get it to work, then take out as much float math as possible
	for (unsigned int iRay = 0; iRay < kResX; ++iRay)
	{
		float rayX = (0.814f)*(iRay - kResX*0.5f);

		const float rotRayX = angCos*rayX - angSin*rayY;
		const float rotRayY = angSin*rayX + angCos*rayY;
		const float X2 = X1+rotRayX;
		const float Y2 = Y1+rotRayY;

		// normalize step value
		float dX = X2-X1;
		float dY = Y2-Y1;
		normalize_vdeltas(dX, dY);

		// counteract fisheye effect (FIXME: real-time parameter?)
		const float fishMul = rayY / sqrtf(rotRayX*rotRayX + rotRayY*rotRayY);
		
		vscape_ray(pDest+iRay, fpX1, fpY1, ftof24(dX), ftof24(dY), fishMul);
	}
}

// -- composition --

bool Landscape_Create()
{
	VIZ_ASSERT(kResX == 800 && kResY == 600); // for HUD

	// load maps
	s_pHeightMap = Image_Load8("assets/scape/maps/D1.png");
	s_pColorMap = Image_Load32("assets/scape/maps/C1W.png");
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

void Landscape_Draw(uint32_t *pDest, float time, float delta)
{
	// render landscape (FIXME: take input)
	memset32(pDest, s_pFogGradient[0], kResX*kResY);
	vscape(pDest, time, delta);

	// overlay HUD
	Add32(pDest, s_pHUD, kResX*kResY);
}

