
// cookiedough -- voxel torus twister

#include "main.h"
// #include "torus-twister.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
#include "boxblur.h"
#include "polar.h"

static uint8_t *s_pHeightMap = NULL;
static uint32_t *s_pColorMap = NULL;
static uint32_t *s_pBeamMap = NULL;

// -- voxel renderer --

// adjust to map resolution
const unsigned kMapSize = 512;
constexpr unsigned kMapAnd = kMapSize-1;                                          
const unsigned kMapShift = 9;

// max. depth
const unsigned int kRayLength = 255;

// height projection table
static unsigned int s_heightProj[kRayLength];

// max. radius (in pixels)
const float kCylRadius = 300.f;

static void vtwister_ray(uint32_t *pDest, int curX, int curY, int dX)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
	__m128i lastColor = c2vISSE(s_pColorMap[U+V]);

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
}

// expected sizes:
// - maps: 512x512
static void vtwister(uint32_t *pDest, float time)
{
	float mapY = 0.f; 
	const float mapStepY = 512.f/kTargetResY; // tile (for blit)

	for (unsigned int iRay = 0; iRay < kTargetResY; ++iRay)
	{
		const float shearAngle = (float) iRay * (2.f*kPI / kTargetResY);

		const int fromX = ftofp24(256.f + 140.f*sinf(time*1.1f + shearAngle));
		const int fromY = ftofp24(mapY + time*25.f);

		const size_t xOffs = kTargetResX>>1;
		vtwister_ray(pDest + iRay*kTargetResX + xOffs, fromX, fromY,  256);
		vtwister_ray(pDest + iRay*kTargetResX + (xOffs), fromX, fromY, -256);

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
}

void Twister_Draw(uint32_t *pDest, float time, float delta)
{
	// render twister
	memset32(g_renderTarget, 0x1f0053, kTargetSize);
	// vtwister(g_renderTarget, time);

	// (radial) blur
	// HorizontalBoxBlur32(g_renderTarget, g_renderTarget, kTargetResX, kTargetResY, 0.04f);

	// polar blit
	Polar_Blit(g_renderTarget, pDest);
}
