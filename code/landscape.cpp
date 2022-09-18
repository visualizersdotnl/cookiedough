
// cookiedough -- voxel landscape

/*
	- more fixed point (+ LUT)
	- procedural fog LUT
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
const float kMapViewLenScale = 0.314f*0.5f;
const int kMapViewHeight = 40;
const int kMapTilt = 190;
const int kMapScale = 500;

// adjust to map resolution
static constexpr unsigned kMapSize = 1024;
constexpr unsigned kMapAnd = kMapSize-1;                                          
constexpr unsigned kMapShift = 10;

// max. depth
const unsigned int kRayLength = 512;

static int s_mapTilt = kMapTilt;

// sample height (filtered)
VIZ_INLINE unsigned int vscape_shf(int U, int V)
{
	unsigned int U0, V0, U1, V1, fracU, fracV;
	bsamp_prepUVs(U, V, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
	return bsamp8(s_pHeightMap, U0, V0, U1, V1, fracU, fracV);
}

static void vscape_ray(uint32_t *pDest, int curX, int curY, int dX, int dY, float fishMul)
{
	int lastHeight = kResY;
	int lastDrawnHeight = kResY;

	const unsigned int U = (curX>>8) & kMapAnd, V = ((curY>>8) & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE16(s_pColorMap[U|V]);

	const int fpFishMul = ftofp24(fabsf(fishMul));
	
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
		height += s_mapTilt;

		VIZ_ASSERT(height >= 0);

		// voxel visible?
		if (height < lastDrawnHeight)
		{
			// draw span (vertical)
			const unsigned int drawLength = lastDrawnHeight - height;
			cspanISSE16(pDest + height*kResX, kResX, lastHeight - height, drawLength, color, lastColor);
			lastDrawnHeight = height;
		}

		lastHeight = height;
		lastColor = color;
	}
}

static void vscape(uint32_t *pDest, float time, float delta)
{
	// grab gamepad input
	PadState pad;
	Gamepad_Update(pad);

	// tilt
	static float tilt = 0.f;
	const float maxTilt = 90.f;
	const float tiltStep = std::min(delta, 1.f);
	if (tilt < maxTilt && pad.rightY > 0.f)
		tilt += tiltStep;
	if (tilt > -maxTilt && pad.rightY < 0.f)
		tilt -= tiltStep;
	s_mapTilt = kMapTilt + int(tilt);

	// calc. view angle + it's sine & cosine (FIXME)
	static float viewAngle = 0.f;
	constexpr float maxAng = kPI*2.f;
	float viewMul = 1.f/kAspect;
	viewMul *= delta*0.01f;
//	if (viewAngle < maxAng)
		viewAngle += viewMul*pad.lShoulder;
//	if (viewAngle > -maxAng)
		viewAngle -= viewMul*pad.rShoulder;

	const float viewCos = cosf(viewAngle);
	const float viewSin = sinf(viewAngle);

	// strafe
	static float strafeX = 0.f;
	static float strafeY = 0.f;
	if (pad.rightX != 0.f)
	{
		const float strafe = pad.rightX*delta;
		strafeX += viewCos*strafe;
		strafeY += viewSin*strafe;
	}

	// move
	static float moveX = 0.f;
	static float moveY = 0.f;
	if (pad.leftY != 0.f)
	{
		const float move = -pad.leftY*delta;
		moveX += -viewSin*move;
		moveY +=  viewCos*move;
	}

	// origin
	float X1 = moveX+strafeX;
	float Y1 = moveY+strafeY;
	const int fpX1 = ftofp24(X1);
	const int fpY1 = ftofp24(Y1);

	for (unsigned int iRay = 0; iRay < kResX; ++iRay)
	{
		constexpr float rayY = kMapSize*kMapViewLenScale;
		const float rayX = 0.25f*(iRay - kResX*0.5f); // FIXME: parameter?

		// FIXME: simplify
		float rotRayX = rayX, rotRayY = rayY;
		voxel::vrot2D(viewCos, viewSin, rotRayX, rotRayY);
		float X2 = X1+rotRayX;
		float Y2 = Y1+rotRayY;
		float dX = X2-X1;
		float dY = Y2-Y1;
		voxel::vnorm2D(dX, dY);

		// counteract fisheye effect (FIXME: real-time parameter?)
		const float fishMul = rayY / sqrtf(rotRayX*rotRayX + rotRayY*rotRayY);
		
		vscape_ray(pDest+iRay, fpX1, fpY1, ftofp24(dX), ftofp24(dY), fishMul);
	}
}

// -- composition --

bool Landscape_Create()
{
//	VIZ_ASSERT(kResX == 800 && kResY == 600); // for HUD

	// load maps
	s_pHeightMap = Image_Load8("assets/scape/maps/D19.png");
	s_pColorMap = Image_Load32("assets/scape/maps/C19w.png");
	s_pHUD = Image_Load32("assets/scape/aircraft_hud.jpg");
	if (nullptr == s_pHeightMap || nullptr == s_pColorMap || nullptr == s_pHUD)
		return false;

	// load fog gradient (8-bit LUT)
	s_pFogGradient = Image_Load32("assets/scape/foggradient.jpg");
	if (s_pFogGradient == NULL)
		return false;
		
	// unpack fog gradient pixels
	for (int iPixel = 0; iPixel < 256; ++iPixel)
		s_fogGradientUnp[iPixel] = c2vISSE16(s_pFogGradient[iPixel]);

	return true;
}

void Landscape_Destroy()
{
}

void Landscape_Draw(uint32_t *pDest, float time, float delta)
{
	// render landscape (FIXME: take input)
	memset32(pDest, s_pFogGradient[0], kResX*kResY);
	vscape(pDest, time, delta);

	// overlay HUD
	// Add32(pDest, s_pHUD, kResX*kResY);
}

