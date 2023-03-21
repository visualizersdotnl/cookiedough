
// cookiedough -- voxel torus twister

#include "main.h"
// #include "torus-twister.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
#include "boxblur.h"
#include "polar.h"
#include "rocket.h"

static uint8_t *s_pHeightMap = nullptr;
static uint32_t *s_pColorMap = nullptr;

static uint32_t *s_pBackground = nullptr;

// Sync.
SyncTrack trackTwisterSpeed;
SyncTrack trackTwisterShearSpeed;
SyncTrack trackTwisterBlur;

// -- voxel renderer --

// adjust to map resolution
const unsigned kMapSize = 1024;
constexpr unsigned kMapAnd = kMapSize-1;                                          
const unsigned kMapShift = 10;

// trace depth (FIXME: parametrize)
const unsigned int kRayLength = 512;

// height projection table
static unsigned s_heightProj[kRayLength];
static unsigned s_heightProjNorm[kRayLength];

// max. radius (in pixels)
const float kCylRadius = 600.f;

static void vtwister_ray(uint32_t *pDest, int curX, int curY, int dX)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	// grab first color
	unsigned int U0, V0, U1, V1, fracU, fracV;
	bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
	__m128i lastColor = bsamp32_16(s_pColorMap, U0, V0, U1, V1, fracU, fracV);

	const int direction = (dX < 0) ? -1 : 1;

	// render ray
	for (unsigned int iStep = 0; iStep < kRayLength; ++iStep)
	{
		// advance!
		curX -= dX; // same flip as in ball.cpp

		// prepare UVs
		unsigned int U0, V0, U1, V1, fracU, fracV;
		bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);

		// fetch height & unpacked color
		const unsigned int mapHeight = bsamp8(s_pHeightMap, U0, V0, U1, V1, fracU, fracV);
		__m128i color = bsamp32_16(s_pColorMap, U0, V0, U1, V1, fracU, fracV);

		// add basic lighting
		const unsigned heightNorm = mapHeight*s_heightProjNorm[iStep] >> 8;
		const unsigned diffuse = (heightNorm*heightNorm) >> 8;
		const __m128i litWhite = _mm_set1_epi16(diffuse);
		color = _mm_adds_epu16(color, litWhite);

		// project height
		const unsigned int height = mapHeight*s_heightProj[iStep] >> 8;

		// voxel visible?
		if (height > lastDrawnHeight)
		{
			// draw span
			const unsigned int drawLength = height - lastDrawnHeight;
			cspanISSE16(pDest, direction, height - lastHeight, drawLength, lastColor, color);
			pDest += int(drawLength)*direction;
			lastDrawnHeight = height;
		}

		lastHeight = height;
		lastColor = color;
	}
}

// expected sizes:
// - maps: 512x512 -> 1024x1024
static void vtwister(uint32_t *pDest, float time)
{
	// tile (for blit)
	constexpr float fMapSize = float(kMapSize);
	constexpr float fMapSizeHH = (fMapSize*0.5f) - 0.5f;
	constexpr float fMapSizeHHH = (fMapSize*0.25f) - 0.5f;
	constexpr float mapStepY = fMapSize/(kTargetResY-1); 

	const float speed = Rocket::getf(trackTwisterSpeed);
	const float shearSpeed = Rocket::getf(trackTwisterShearSpeed);

	#pragma omp parallel for schedule(static)
	for (int iRay = 0; iRay < kTargetResY; ++iRay)
	{
		const float shearAngle = (float) iRay * (k2PI/(kTargetResY-1));

		const float mapY = iRay*mapStepY;
		const int fromX = ftofp24(fMapSizeHH + fMapSizeHHH*sinf(time*shearSpeed + shearAngle));
		const int fromY = ftofp24(mapY + time*speed);

		const size_t xOffs = iRay*kTargetResX + (kTargetResX>>1);
		vtwister_ray(pDest+xOffs, fromX, fromY,  kMapSize/2);
		vtwister_ray(pDest+xOffs-1, fromX- kMapSize/2, fromY, -(kMapSize/2));
	}
}

void vtwister_precalc()
{
	// heights along ray wrap around half a circle
	for (unsigned int iAngle = 0; iAngle < kRayLength; ++iAngle)
	{
		const float angle = kPI/(kRayLength-1) * iAngle;
		const float scale = kCylRadius*sinf(angle);
		s_heightProj[iAngle] = (unsigned) scale;
		
		// for basic lighting
		const float cosine = cosf(angle*0.99f);
		if (cosine > 0.f)
			s_heightProjNorm[iAngle] = unsigned(255.f*cosine);
		else
			s_heightProjNorm[iAngle] = 0;
	}
}

// -- composition --

bool Twister_Create()
{
	vtwister_precalc();

	// load maps
	s_pHeightMap = Image_Load8("assets/twister/hmap_2_1k.jpg");
	s_pColorMap = Image_Load32("assets/twister/colormap_1k.jpg");
	if (nullptr == s_pHeightMap || nullptr == s_pColorMap)
		return false;

	// load background (1280x720)
	s_pBackground = Image_Load32("assets/twister/nytrik-background_1280x720.png");
	if (nullptr == s_pBackground)
		return false;

	// init. sync.
	trackTwisterSpeed = Rocket::AddTrack("twister:Speed");
	trackTwisterShearSpeed = Rocket::AddTrack("twister::ShearSpeed");
	trackTwisterBlur = Rocket::AddTrack("twister:Blur");

	return true;
}

void Twister_Destroy()
{
}

void Twister_Draw(uint32_t *pDest, float time, float delta)
{
	// render twister 
	memset32(g_renderTarget[0], 0, kTargetSize); // FIXME
	vtwister(g_renderTarget[0], time);

	// (radial) blur
	const float blur = Rocket::getf(trackTwisterBlur);
	if (0.f != blur)
	{
		const float scaledBlur = BoxBlurScale(blur);
		HorizontalBoxBlur32(g_renderTarget[0], g_renderTarget[0], kTargetResX, kTargetResY, scaledBlur);
	}

	// blit background (FIXME)
	memcpy(pDest, s_pBackground, kOutputBytes);

	// polar blit
	Polar_BlitA(pDest, g_renderTarget[0]);

	// debug blit (vertical)
//	memcpy(pDest, g_renderTarget[0], kOutputBytes);
}
