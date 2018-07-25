
// cookiedough -- voxel landscape (640x480)

/*
	- to fixed point (vscape_ray(), mostly)
	- some XInput fanciness?
*/

#include "main.h"
// #include "landscape.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
#include "shared.h"
#include "polar.h"
#include "boxblur.h"

static uint8_t *s_pHeightMap = NULL;
static uint32_t *s_pColorMap = NULL;
static uint32_t *s_pFogGradient = NULL;

static __m128i s_fogGradientUnp[256];

// -- voxel renderer --

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

static void vscape_ray(uint32_t *pDest, int curX, int curY, int dX, int dY, float D)
{
	int lastHeight = kResY;
	int lastDrawnHeight = kResY;

	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = bsamp32(s_pColorMap, U, V, U, V, 0, 0);
	
	for (unsigned int iStep = 1; iStep <= kRayLength; ++iStep)
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

		// apply fog (additive, no clamp: can overflow)
		color = _mm_adds_epu16(color, s_fogGradientUnp[iStep-1]);
		
		float fHeight = 256.f-mapHeight;
		fHeight = fHeight - 128.f;
		fHeight += -50.f;
		fHeight /= fabsf(D)*iStep;
		fHeight *= 160.f;
		fHeight -= -150.f;

		int height = (int) fHeight;
		if (height < 0) height = 0;

		// voxel visible?
		if (height < lastDrawnHeight)
		{
			const unsigned int drawLength = lastDrawnHeight - height;

			// draw span (vertical)
//			cspanISSE_noclip(pDest + height*kResX, kResX, lastDrawnHeight - height, color, lastColor);
			cspanISSE(pDest + height*kResX, kResX, lastHeight - height, drawLength, color, lastColor);
			lastDrawnHeight = height;
			lastColor = color;
		}

		// for later... (clipping)
		lastHeight = height;
		lastColor = color;
	}
}

// expected sizes:
// - maps: 1024x1024
static void vscape(uint32_t *pDest, float time)
{
	float viewAngle = time*0.0314f;

	float origX = 512.f;
	float origY = 800.f + time*19.f;

	float rayY = 270.f;

	for (unsigned int iRay = 0; iRay < kResX; ++iRay)
	{
		float rayX = 1.5f*(iRay - kResX*0.5f);

		float rotX = cosf(viewAngle)*rayX + sinf(viewAngle)*rayY;
		float rotY = -sin(viewAngle)*rayX + cos(viewAngle)*rayY;

		float X1 = origX;
		float X2 = origX+rotX;
		float Y1 = origY;
		float Y2 = origY+rotY;

		float dX = X2-X1;
		float dY = Y2-Y1;
		float r = sqrtf(dX*dX + dY*dY);
		dX /= r;
		dY /= r;

		float D = rayY / sqrtf(rayX*rayX + rayY*rayY);

		vscape_ray(pDest+iRay, ftof24(X1+dX), ftof24(Y1+dY), ftof24(dX), ftof24(dY), D);

	}
}

// -- composition --

bool Landscape_Create()
{
	// load maps
	s_pHeightMap = Image_Load8("assets/scape/D1.png");
	s_pColorMap = Image_Load32("assets/scape/C1W.png");
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

void Landscape_Destroy()
{
	delete[] s_pHeightMap;
	delete[] s_pColorMap;
	delete[] s_pFogGradient;
}

void Landscape_Draw(uint32_t *pDest, float time)
{
	// FIXME
	memset(pDest, s_pFogGradient[255], kResX*kResY*4);

	vscape(pDest, time);
}
