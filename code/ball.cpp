
// cookiedough -- voxel ball (2-pass approach)

#include "main.h"
// #include "ball.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
#include "boxblur.h"
#include "polar.h"
#include "voxel-shared.h"
#include "rocket.h"
// #include "shadertoy-util.h"

static uint8_t *s_pHeightMap[5] = { nullptr };
static uint32_t *s_pColorMap[2] = { nullptr };
static uint32_t *s_pBeamMap = nullptr;
static uint32_t *s_pEnvMap = nullptr;
static uint32_t *s_pBackground = nullptr;

static uint8_t *s_heightMapMix = nullptr;

// FIXME: temporary fun, please remove
static uint32_t *s_pOrange = nullptr;

// --- Sync. tracks ---

SyncTrack trackBallBlur;
SyncTrack trackBallRadius; 
SyncTrack trackBallRayLength;
SyncTrack trackBallSpikes;
SyncTrack trackBallHasBeams;
SyncTrack trackBallBaseShapeIndex;

// --------------------
	
// -- voxel renderer --

// things that need attention:
// - environment mapping & basic lighting, including Hoplite's "specular" (WIP)
// - proper background visibility/blending
// - move object around
// - orange were doing something to curtail yet unify the beams: figure out what

#define DEBUG_BALL_LIGHTING

// adjust to map resolution
constexpr unsigned kMapSize = 512;
constexpr unsigned kMapAnd = kMapSize-1;                                          
constexpr unsigned kMapShift = 9;

// max. trace depth
constexpr unsigned kMaxRayLength = 512;

// height projection table
static unsigned s_heightProj[kMaxRayLength];
static unsigned s_curRayLength = kMaxRayLength;

// max. radius (in pixels)
constexpr float kMaxBallRadius = float((kResX > kResY) ? kResX : kResY);

// applied to each beam sample
constexpr auto kBeamMod = 6;

static void vball_ray_beams(uint32_t *pDest, int curX, int curY, int dX, int dY)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	unsigned int U0, V0, U1, V1, fracU, fracV;
	bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
	__m128i lastColor = bsamp32_16(s_pColorMap[0], U0, V0, U1, V1, fracU, fracV);
//	*pDest = v2cISSE16(lastColor);

//	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
//	__m128i lastColor = c2vISSE16(s_pColorMap[0][U+V]);

	const __m128i beamMod = g_gradientUnp[kBeamMod];
	const __m128i beam = bsamp32_16(s_pBeamMap, U0, V0, U1, V1, fracU, fracV);
	__m128i beamAccum =  _mm_srli_epi16(_mm_mullo_epi16(beam, beamMod), 8);

	for (unsigned int iStep = 0; iStep < s_curRayLength; ++iStep)
	{
		// I had the mapping the wrong way around (or I am adjusting for something odd elsewhere, but it works for now (FIXME))
		curX -= dX;
		curY -= dY;

		// prepare UVs
		unsigned int U0, V0, U1, V1, fracU, fracV;
		bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);

		// fetch height & unpacked color
		const unsigned int mapHeight = bsamp8(s_heightMapMix, U0, V0, U1, V1, fracU, fracV);
		__m128i color = bsamp32_16(s_pColorMap[0], U0, V0, U1, V1, fracU, fracV);

		// and beam (extrusion) color (while the prepared UVs are still intact)
		__m128i beam = bsamp32_16(s_pBeamMap, U0, V0, U1, V1, fracU, fracV);

		// accumulate & add beam (separate map)
		beamAccum = _mm_adds_epu16(beamAccum, _mm_srli_epi16(_mm_mullo_epi16(beam, beamMod), 8));
		color = _mm_adds_epu16(color, beamAccum);

		// project height
		const unsigned int height = mapHeight*s_heightProj[iStep] >> 8;

		// voxel visible?
		if (height > lastDrawnHeight)
		{
			const unsigned int drawLength = height - lastDrawnHeight;

			// draw span (clipped)
			cspanISSE16(pDest + lastDrawnHeight, 1, height - lastHeight, drawLength, lastColor, color);
			lastDrawnHeight = height;
		}

		lastHeight = height;
		lastColor = color;
	}

	// beam fade out
	const unsigned int remainder = (kTargetResX-1)-lastDrawnHeight;
	cspanISSE16_noclip(pDest + lastDrawnHeight, 1, remainder, beamAccum, _mm_setzero_si128()); 
}

static void vball_ray_no_beams(uint32_t *pDest, int curX, int curY, int dX, int dY)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	unsigned int U0, V0, U1, V1, fracU, fracV;
	bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
	__m128i lastColor = bsamp32_16(s_pColorMap[1], U0, V0, U1, V1, fracU, fracV);
//	*pDest = v2cISSE16(lastColor);

//	const unsigned int U = curX >> 8 & kMapAnd, V = (curY >> 8 & kMapAnd) << kMapShift;
//	__m128i lastColor = c2vISSE16(s_pColorMap[U+V]);

	int envU = ((kMapSize>>1))<<8;
	int envV = envU;

	for (unsigned int iStep = 0; iStep < s_curRayLength; ++iStep)
	{
		// I had the mapping the wrong way around (or I am adjusting for something odd elsewhere, but it works for now (FIXME))
		curX -= dX;
		curY -= dY;

		// step twice as much through the env. map
		envU -= dX<<1;
		envV -= dY<<1;

		// prepare UVs
		unsigned int U0, V0, U1, V1, fracU, fracV;
		bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);

		// fetch height & unpacked color
		const unsigned int mapHeight = bsamp8(s_heightMapMix, U0, V0, U1, V1, fracU, fracV);
		__m128i color = bsamp32_16(s_pColorMap[1], U0, V0, U1, V1, fracU, fracV);

		// sample env. map (in an unorthodox way that looks good enough)
		bsamp_prepUVs(envU + mapHeight, envV + mapHeight, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
		const __m128i envCol = bsamp32_16(s_pEnvMap, U0, V0, U1, V1, fracU, fracV);

		// project height
		const unsigned int height = mapHeight*s_heightProj[iStep] >> 8;

		// lighting
		const unsigned diffuse = std::min<unsigned>(65535, unsigned(powf(mapHeight, 2.f))) >> 8; // FIXME: use LUT or something else faster
		__m128i lit = _mm_set1_epi16(diffuse);

#if defined(DEBUG_BALL_LIGHTING)

		color = lit;
		color = _mm_adds_epu16(c2vISSE16(0xff000000), color); // no blending please

#else

		color = _mm_adds_epu16(color, _mm_srli_epi16(_mm_mullo_epi16(envCol, lit), 8));

#endif

		// voxel visible?
		if (height > lastDrawnHeight)
		{
			const unsigned int drawLength = height - lastDrawnHeight;

			// draw span (clipped)
			cspanISSE16(pDest + lastDrawnHeight, 1, height - lastHeight, drawLength, lastColor, color);
			lastDrawnHeight = height;
		}

		lastHeight = height;
		lastColor = color;
	}

	while (lastDrawnHeight < kTargetResX)
		pDest[lastDrawnHeight++] = 0;
}

static void vball_precalc()
{
	const float radius = clampf(1.f, kMaxBallRadius, Rocket::getf(trackBallRadius));
	const float fRayLength = clampf(1.f, kMaxRayLength, Rocket::getf(trackBallRayLength)); // FIXME: use 'geti()'
	s_curRayLength = unsigned(fRayLength);

	// heights along ray wrap around half a circle
	const float angStep = kPI/(s_curRayLength-1);
	for (unsigned int iAngle = 0; iAngle < s_curRayLength; ++iAngle)
	{
		const float angle = angStep*iAngle;
		const float scale = radius*sinf(angle);
		s_heightProj[iAngle] = (unsigned) scale;
	}
}

// expected sizes:
// - maps: 512x512
static void vball(uint32_t *pDest, float time)
{
	// precalc. projection map (FIXME: it's just a multiplication and a sine, can't we move this to the ray function already?)
	vball_precalc();

	// select if has beams
	void (*vball_ray_fn)(uint32_t *, int, int, int, int);
	vball_ray_fn = Rocket::geti(trackBallHasBeams) != 0 ? &vball_ray_beams : &vball_ray_no_beams;

	// move ray origin to fake hacky rotation (FIXME)
	const int fromX = ftofp24(512.f*cosf(time*0.25f) + 256.f);
	const int fromY = ftofp24(512.f*sinf(time*0.25f) + 256.f);

	// FOV (full circle)
	constexpr float fovAngle = kPI*2.f;
	constexpr float delta = fovAngle/kTargetResY;
//	float curAngle = 0.f;

	// cast rays
	#pragma omp parallel for schedule(static)
	for (int iRay = 0; iRay < kTargetResY; ++iRay)
	{
		float curAngle = iRay*delta;
		float dX, dY;
		voxel::calc_fandeltas(curAngle, dX, dY);
		vball_ray_fn(pDest + iRay*kTargetResX, fromX, fromY, ftofp24(dX), ftofp24(dY));
//		curAngle += delta;
	}
}

// -- composition --

const char *kHeightMapPaths[5] =
{
	// spikey thing
	"assets/ball/by-orange/hmap_1.jpg",  

	// ball
	"assets/ball/by-orange/hmap_4.jpg",    

	// misc.
	"assets/ball/by-orange/hmap_2.jpg", 
	"assets/ball/by-orange/hmap_3.jpg", 
	"assets/ball/by-orange/hmap_5.jpg", 
};

bool Ball_Create()
{
	// load height maps
	for (int iMap = 0; iMap < 5; ++iMap)
	{
		s_pHeightMap[iMap] = Image_Load8(kHeightMapPaths[iMap]);
		if (nullptr == s_pHeightMap[iMap])
			return false;
	}
	
	// load color maps
	s_pColorMap[0] = Image_Load32("assets/ball/by-orange/colormap.jpg"); // used as base map when beams active
	s_pColorMap[1] = Image_Load32("assets/ball/by-orange/envmap2.jpg");  // used otherwise
	if (nullptr == s_pColorMap[0] || nullptr == s_pColorMap[1])
		return false;

	// load beam map (pairs with 'assets/ball/by-orange/colormap.jpg')
	s_pBeamMap = Image_Load32("assets/ball/by-orange/beammap.jpg");
	if (nullptr == s_pBeamMap)
		return false;

	// load env. map
	s_pEnvMap = Image_Load32("assets/ball/by-orange/envmap3.jpg");
//	s_pEnvMap = Image_Load32("assets/ball/by-orange/envmap2.jpg");
	if (nullptr == s_pEnvMap)
		return false;

	// load background (1280x720)
	s_pBackground = Image_Load32("assets/ball/background_1280x720.png");
	if (nullptr == s_pBackground)
		return false;

	s_heightMapMix = static_cast<uint8_t*>(mallocAligned(512*512*sizeof(uint8_t), kAlignTo));

	// initialize sync. track(s)
	trackBallBlur = Rocket::AddTrack("ballBlur");
	trackBallRadius = Rocket::AddTrack("ballRadius");
	trackBallRayLength = Rocket::AddTrack("ballRayLength");
	trackBallSpikes = Rocket::AddTrack("ballSpikes");
	trackBallHasBeams = Rocket::AddTrack("ballHasBeams");
	trackBallBaseShapeIndex = Rocket::AddTrack("ballBaseShapeIndex");

	// FIXME
	s_pOrange = Image_Load32_CA("assets/by-orange/x37.jpg", "assets/by-orange/x40.jpg");
	if (nullptr == s_pOrange)
		return false;

	return true;
}

void Ball_Destroy()
{
	freeAligned(s_heightMapMix);
}

void Ball_Draw(uint32_t *pDest, float time, float delta)
{
	// blend between map (1-4) and and #0 (spikes)
	const unsigned iBaseMap = clampi(1, 4, Rocket::geti(trackBallBaseShapeIndex));
	memcpy_fast(s_heightMapMix, s_pHeightMap[iBaseMap], 512*512);

	const uint8_t spikes = (uint8_t) clampi(0, 255, Rocket::geti(trackBallSpikes));
	if (spikes != 0)
		Mix32(reinterpret_cast<uint32_t *>(s_heightMapMix), reinterpret_cast<uint32_t*>(s_pHeightMap[0]), 512*512/4 /* function processes 4 8-bit components at a time */, spikes);

	// render unwrapped ball
	vball(g_renderTarget[0], time);

#if 1
	// blur (optional)
	float blur = Rocket::getf(trackBallBlur);
	if (blur >= 1.f && blur <= 100.f)
	{
		blur *= kBoxBlurScale;
		HorizontalBoxBlur32(g_renderTarget[0], g_renderTarget[0], kResX, kResY, blur);
	}
#endif

	// polar blit
	Polar_Blit(g_renderTarget[1], g_renderTarget[0], false);

	// put it together (FIXME)
	memcpy(pDest, s_pBackground, kOutputBytes);
//	MixSrc32(pDest, g_renderTarget[1], kResX*kResY);
	Add32(pDest, g_renderTarget[1], kResX*kResY);

	// FIXME
	BlitSrc32(pDest + (((kResY-384)>>1)+100)*kResX + (((kResX-512)>>1)+150), s_pOrange, kResX, 512, 384);

#if 0
	// debug blit: unwrapped
	const uint32_t *pSrc = g_renderTarget[0];
	for (unsigned int iY = 0; iY < kResY; ++iY)
	{
		memcpy(pDest, pSrc, kTargetResX*4);
		pSrc += kTargetResX;
		pDest += kResX;
	}
#endif
}
