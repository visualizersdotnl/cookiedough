
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
static uint32_t* s_pBackgrounds[2] = { nullptr };

static uint8_t *s_heightMapMix = nullptr;

// --- Sync. tracks ---

SyncTrack trackBallBlur;
SyncTrack trackBallRadius; 
SyncTrack trackBallRayLength;
SyncTrack trackBallSpikes;
SyncTrack trackBallHasBeams;
SyncTrack trackBallBaseShapeIndex;
SyncTrack trackBallSpeed;
SyncTrack trackBallBeamAtten;
SyncTrack trackBallBeamAlphaMin;
SyncTrack trackBallRotateOffsX, trackBallRotateOffsY;

// --------------------
	
// -- voxel renderer --

// things that need attention:
// - use 16-bit height map(s)
// - ...

// see implementation
// #define DEBUG_BALL_LIGHTING
// #define USE_LAST_BEAM_ACCUM

// adjust to map resolution
constexpr unsigned kMapSize = 1024;
constexpr unsigned kMapAnd = kMapSize-1;                                          
constexpr unsigned kMapShift = 10;

// max. trace depth
constexpr unsigned kMaxRayLength = 1024;

// height projection table
static unsigned s_heightProj[kMaxRayLength];
static int s_heightProjNorm[kMaxRayLength]; // for "lighting", projects on quarter of a circle, to attenuate and cull
static unsigned s_curRayLength = kMaxRayLength;

// max. radius (in pixels)
// constexpr float kMaxBallRadius = float((kResX > kResY) ? kResX : kResY);
constexpr float kMaxBallRadius = 1920.f;

// beam attenuation (during accumulation) [0..255]
constexpr auto kDefaultBeamAtten = 64;
static unsigned s_beamAtten = kDefaultBeamAtten;

// minimum beam alpha (extrusion) [0..255]
constexpr float kDefaultBeamAlphaMin = 10.f;
static float s_beamAlphaMin = kDefaultBeamAlphaMin;

// ambient added to light calc.
constexpr unsigned kAmbient = 32;

static void vball_ray_beams(uint32_t *pDest, int curX, int curY, int dX, int dY)
{
	unsigned int lastHeight = 0;
	unsigned int lastDrawnHeight = 0; 

	// grab first color
	unsigned int U0, V0, U1, V1, fracU, fracV;
	bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
	__m128i lastColor = bsamp32_16(s_pColorMap[0], U0, V0, U1, V1, fracU, fracV);

	// init. beam accumulator(s)
	__m128i beamAccum = _mm_setzero_si128();

#if defined(USE_LAST_BEAM_ACCUM)

	__m128i lastBeamAccum = _mm_setzero_si128();

#endif

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

		// and beam (light shaft) color
		__m128i beam = bsamp32_16(s_pBeamMap, U0, V0, U1, V1, fracU, fracV);
		beam = _mm_srli_epi16(_mm_mullo_epi16(beam, g_gradientUnp16[s_beamAtten]), 8);

		// light beam
		const unsigned heightNorm = mapHeight*s_heightProjNorm[iStep] >> 8;
		const unsigned diffuse = (heightNorm*heightNorm) >> 8;
		const __m128i litWhite = _mm_set1_epi16(diffuse);
//		const __m128i litPurple = c2vISSE16(diffuse*0x01010001); 
		const __m128i beamLit = _mm_srli_epi16(_mm_mullo_epi16(beam, litWhite), 8);
		beamAccum = _mm_adds_epu16(beamAccum, beamLit);

#if defined(DEBUG_BALL_LIGHTING)

		color = litWhite;
		color = _mm_adds_epu16(c2vISSE16(0xff<<24), color); // no blending please

#endif

		// add beam & very simple directional light to color
		color = _mm_adds_epu16(color, beamAccum);
		color = _mm_adds_epu16(color, litWhite);

		// project height
		const unsigned int height = mapHeight*s_heightProj[iStep] >> 8;

		// voxel visible?
		if (height > lastDrawnHeight)
		{
			const unsigned int drawLength = height - lastDrawnHeight;

			// draw span (clipped)
			cspanISSE16(pDest + lastDrawnHeight, 1, height - lastHeight, drawLength, lastColor, color);
			lastDrawnHeight = height;

#if defined(USE_LAST_BEAM_ACCUM)

			// only extrude from the last drawn pixel onwards
			lastBeamAccum = beamAccum;

#endif
		}

		lastHeight = height;
		lastColor = color;
	}

	// beam extrusion
	// FIXME: this *should* be faster code, but for now it's sufficient

#if defined(USE_LAST_BEAM_ACCUM)
	unsigned beamCol = v2cISSE16(lastBeamAccum);
#else
	unsigned beamCol = v2cISSE16(beamAccum);
#endif

	const unsigned remainder = (kTargetResX - 1) - lastDrawnHeight;

	// discard alpha
	beamCol &= 0xffffff;

	const auto beamR = beamCol >> 16;        
	const auto beamG = (beamCol >> 8) & 0xff;
	const auto beamB = beamCol & 0xff;        

	// NTSC weights
	constexpr unsigned mulR = unsigned(0.0722f*65536.f);
	constexpr unsigned mulG = unsigned(0.7152f*65536.f);
	constexpr unsigned mulB = unsigned(0.2126f*65536.f);

	const unsigned luminosity = ((beamR*mulR) >> 16) + ((beamG*mulG) >> 16) + ((beamB*mulB) >> 16);
	const float fLuminosity = float(luminosity);

	const float alphaStep = 1.f / (remainder - 1);
	float curStep = 0.f;
	for (unsigned iPixel = 0; iPixel < remainder; ++iPixel)
	{
		const float fBeamAlpha = smoothstepf(s_beamAlphaMin, fLuminosity, curStep);
		const unsigned beamAlpha = unsigned(fBeamAlpha);
		pDest[lastDrawnHeight++] = beamCol | (beamAlpha << 24);
		curStep += alphaStep;
	}
}

static void vball_ray_no_beams(uint32_t *pDest, int curX, int curY, int dX, int dY)
{
	// grab first color
	unsigned int U0, V0, U1, V1, fracU, fracV;
	bsamp_prepUVs(curX, curY, kMapAnd, kMapShift, U0, V0, U1, V1, fracU, fracV);
	__m128i lastColor = bsamp32_16(s_pColorMap[1], U0, V0, U1, V1, fracU, fracV);

	int envU = ((kMapSize>>1))<<8;
	int envV = envU;

	unsigned lastHeight = 0;
	unsigned lastDrawnHeight = 0;

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
		const unsigned heightNorm = mapHeight*s_heightProjNorm[iStep] >> 8;

		const unsigned diffuse = (heightNorm*heightNorm*heightNorm)>>16;
		const __m128i lit = _mm_set1_epi16(diffuse);

		const unsigned fullyLit = std::min<unsigned>(255, kAmbient+diffuse);
		const __m128i litFull = _mm_set1_epi16(fullyLit);

#if defined(DEBUG_BALL_LIGHTING)

		color = litFull;
		color = _mm_adds_epu16(c2vISSE16(0xff<<24), color); // no blending please

#else

		color = _mm_adds_epu16(color, _mm_srli_epi16(_mm_mullo_epi16(envCol, litFull), 8));
		color = _mm_adds_epu16(color, lit); // extra shine

#endif

		// voxel visible?
		if (height > lastDrawnHeight)
		{
			const unsigned int drawLength = height - lastDrawnHeight;
			const unsigned int length = height - lastHeight;

			// draw span (clipped)
			cspanISSE16(pDest + lastDrawnHeight, 1, length, drawLength, lastColor, color);
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
	s_curRayLength = clampi(1, kMaxRayLength, Rocket::geti(trackBallRayLength));

	// heights along ray wrap around half a circle
	const float angStepSin = kPI/(s_curRayLength-1);
	const float angStepCos = angStepSin * 0.99f;
	for (unsigned iAngle = 0; iAngle < s_curRayLength; ++iAngle)
	{
		// sine goes from 0 to 1 back to 0 for [0..kPI] so as to fold it around half a sphere
		s_heightProj[iAngle] = unsigned(radius*sinf(angStepSin*iAngle));    

		// cosine goes from 1 down to -1, effectively culling
		const float cosine = cosf(angStepCos*iAngle);
		(cosine >= 0.f)
			? s_heightProjNorm[iAngle] = int(255.f*cosine)
			: s_heightProjNorm[iAngle] = 0; // lighting calc. does not take kindly to negative (signed)
	}
}

// expected sizes:
// - maps: 1024x1024
static void vball(uint32_t *pDest, float time)
{
	// precalc. projection map (FIXME: it's just a multiplication and a sine, can't we move this to the ray function already?)
	vball_precalc();

	// set global parameters
	s_beamAtten = clampi(0, 255, Rocket::geti(trackBallBeamAtten));
	s_beamAlphaMin = clampf(0.f, 255.f, Rocket::getf(trackBallBeamAlphaMin));

	// select if has beams
	void (*vball_ray_fn)(uint32_t *, int, int, int, int);
	vball_ray_fn = Rocket::geti(trackBallHasBeams) != 0 ? &vball_ray_beams : &vball_ray_no_beams;

	// move ray origin to fake hacky rotation 
	const float timeScale = s_curRayLength*(0.25f/kMaxRayLength);
	const float fMapDim = float(kMapSize);
	const float fMapHalf = fMapDim*0.5f;
	const int fromX = ftofp24(fMapDim*cosf(time*timeScale) + fMapHalf + Rocket::getf(trackBallRotateOffsX));
	const int fromY = ftofp24(fMapDim*sinf(time*timeScale) + fMapHalf + Rocket::getf(trackBallRotateOffsY));

	// FOV (full circle)
	constexpr float fovAngle = kPI*2.f;
	constexpr float delta = fovAngle/(kTargetResY-1);
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
	"assets/ball/hmap_1_1k.jpg",  

	// ball
	"assets/ball/hmap_4_1k.jpg",    

	// misc.
	"assets/ball/hmap_2_1k.jpg", 
	"assets/ball/hmap_3_1k.jpg", 
	"assets/ball/hmap_5_1k.jpg", 
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
	s_pColorMap[0] = Image_Load32("assets/ball/colormap_1k.jpg"); // used as base map when beams active
	s_pColorMap[1] = Image_Load32("assets/ball/colormap_2_1k.jpg");  // used otherwise
	if (nullptr == s_pColorMap[0] || nullptr == s_pColorMap[1])
		return false;

	// load beam map (pairs with 'assets/ball/*/colormap_1k.jpg')
	s_pBeamMap = Image_Load32("assets/ball/beammap_1k.jpg");
	if (nullptr == s_pBeamMap)
		return false;

	// load env. map
	s_pEnvMap = Image_Load32("assets/ball/envmap3_1k.jpg");
	if (nullptr == s_pEnvMap)
		return false;

	// load backgrounds (1280x720)
	s_pBackgrounds[0] = Image_Load32("assets/ball/nytrik-background_1280x720.png");
	s_pBackgrounds[1] = Image_Load32("assets/ball/nytrik-background-2-1280x720.png");
	if (nullptr == s_pBackgrounds[0] || nullptr == s_pBackgrounds[1])
		return false;

	s_heightMapMix = static_cast<uint8_t*>(mallocAligned(kMapSize*kMapSize*sizeof(uint8_t), kAlignTo));

	// initialize sync. track(s)
	trackBallBlur = Rocket::AddTrack("ball:Blur");
	trackBallRadius = Rocket::AddTrack("ball:Radius");
	trackBallRayLength = Rocket::AddTrack("ball:RayLength");
	trackBallSpikes = Rocket::AddTrack("ball:Spikes");
	trackBallHasBeams = Rocket::AddTrack("ball:HasBeams");
	trackBallBaseShapeIndex = Rocket::AddTrack("ball:BaseShapeIndex");
	trackBallSpeed = Rocket::AddTrack("ball:Speed");
	trackBallBeamAtten = Rocket::AddTrack("ball:BeamAttenuation");
	trackBallBeamAlphaMin = Rocket::AddTrack("ball:BeamAlphaMin");
	trackBallRotateOffsX = Rocket::AddTrack("ball:RotateOffsX");
	trackBallRotateOffsY = Rocket::AddTrack("ball:RotateOffsY");

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
	memcpy_fast(s_heightMapMix, s_pHeightMap[iBaseMap], kMapSize*kMapSize);

	const uint8_t spikes = (uint8_t) clampi(0, 255, Rocket::geti(trackBallSpikes));
	if (spikes != 0)
		Mix32(reinterpret_cast<uint32_t *>(s_heightMapMix), reinterpret_cast<uint32_t*>(s_pHeightMap[0]), kMapSize*kMapSize/4 /* function processes 4 8-bit components at a time */, spikes);

	// render unwrapped ball
	vball(g_renderTarget[0], time * Rocket::getf(trackBallSpeed));

#if 1
	// blur (optional)
	const float blur = BoxBlurScale(Rocket::getf(trackBallBlur));
	if (0.f != blur)
		HorizontalBoxBlur32(g_renderTarget[0], g_renderTarget[0], kResX, kResY, blur);
#endif

	// blit (polar wrap) effect on top of background (2 of them, one for the object *with* beams, one for without)
	const bool hasBeams = Rocket::geti(trackBallHasBeams) != 0;
	const auto* pBackground = hasBeams ? s_pBackgrounds[0] : s_pBackgrounds[1];
	memcpy(pDest, pBackground, kOutputBytes);
//	memset32(pDest, 0, kOutputSize);
	Polar_BlitA(pDest, g_renderTarget[0], false);

	// and then some
	if (true == hasBeams)
		SoftLight32AA(pDest, pBackground, kOutputSize, 0.4f);

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

uint32_t *Ball_GetBackground()
{
	VIZ_ASSERT(nullptr != s_pBackgrounds[0]);
	return s_pBackgrounds[0];
}

bool Ball_HasBeams()
{
	return Rocket::geti(trackBallHasBeams) != 0;
}