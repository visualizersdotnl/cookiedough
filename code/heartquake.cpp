
// cookiedough -- Heartquake-style water (see https://github.com/visualizersdotnl/heartquake-water-explained)

#include "main.h"
// #include "heartquake.h"
#include "image.h"
#include "cspan.h"
#include "bilinear.h"
// #include "boxblur.h"
// #include "polar.h"

static int32_t *s_pWater[2] = { nullptr };

static int s_waterFlip = 0;

// -- implementation --

static void integrate()
{
	// grab buffers
	int32_t *pA = s_pWater[s_waterFlip];
	int32_t *pB = s_pWater[s_waterFlip^1];

	// integrate
	for (unsigned iY = 1; iY < kResY-1; ++iY)
	{
		for (unsigned iX = 0; iX < kResX; ++iX)
		{
			// just so we're ready for OpenMP
			const auto index = iX + iY*kResX;

			const int32_t accumAvg = (pA[index-1] + pA[index+1] + pA[index-kResX] + pA[index+kResX]) >> 1;
			const int32_t current  = pB[index];
			const int32_t delta = accumAvg-current;
			const int32_t dampened = delta>>5; // FIXME: make variable!
			const int32_t result = delta-dampened;
			pB[index] = result;
		}
	}
}

// -- composition --

bool Heartquake_Create()
{
	// allocate water buffers
	constexpr auto gridSize = kResX*kResY*sizeof(int32_t);
	s_pWater[0] = static_cast<int32_t*>(mallocAligned(gridSize, kAlignTo));
	s_pWater[1] = static_cast<int32_t*>(mallocAligned(gridSize, kAlignTo));

	// level 'em
	for (auto* pointer : s_pWater)
		memset(pointer, 0, gridSize);

	return true;
}

void Heartquake_Destroy()
{
	for (auto* pointer : s_pWater)
		freeAligned(pointer);
}

void Heartquake_Draw(uint32_t *pDest, float time, float delta)
{
	// integrate 
	integrate();

	// animate (FIXME)
	constexpr float trailGranularity = 512.f;
	int32_t *pWrite = s_pWater[s_waterFlip^1];
	for (unsigned iTrail = 0; iTrail < 2; ++iTrail)
	{
		// simple sinus snakes
		const float phase = iTrail*(k2PI/trailGranularity) + time*0.25f;
		float cosY = 0.5f + cosf(phase + sinf(time))*0.5f;
		float sinX = 0.5f + sinf(phase + cosf(time*1.3f))*0.5f;

		// keep within boundaries
		cosY = (0.1f+cosY*0.8f)*kResY;
		sinX = (0.05f+sinX*0.9f)*kResX;

		// cast
		int iY = int(cosY);
		int iX = int(sinX);

		const auto iPixel = iX + iY*kResX;
		constexpr int32_t depth = 1024;

		// write 2x2 block as Iguana did
		pWrite[iPixel]       += depth;
//		pWrite[iPixel+1]     += depth;
//		pWrite[iPixel-kResX] += depth;
//		pWrite[iPixel+kResX] += depth;
	}

	// blit first (debug)
	constexpr auto gridSize = kResX*kResY;
	const int32_t *pRead = s_pWater[s_waterFlip];
	for (auto iPixel = 0; iPixel < gridSize; ++iPixel)
	{
		const int32_t depth = *pRead++;

		// FIXME
		uint32_t pixel = 0;
		if (depth > 255)
			pixel = 0xffffff;
		else
			pixel = depth*0x010101;
		if (depth < 0)
			pixel = 0;

		*pDest++ = pixel;
	}

	s_waterFlip^=-1;
	return;

	// blit ("projected")
	memset(pDest, 0, kResX*kResY*sizeof(uint32_t));
	for (int iX = 0; iX < kResX; ++iX)
	{
		int32_t lastHeight = 7;
		int32_t curPos = kResY-1;
		uint32_t curColor = 255;

		for (int iY = kResY-1; iY >= 0; --iY)
		{
			int32_t curHeight = lastHeight-8;
			int32_t curWaterDepth = pRead[iX + iY*kResX];
			int32_t delta = curWaterDepth-curHeight;
			if (delta > curHeight)
			{
				delta |= 0x7;
				while (delta >= 0)
				{
					pDest[iX + curPos*kResX] = curColor*0x010101;
					--curPos;
					if (curColor > 0) --curColor;
					delta -= 8;
				}
			}
		}
	}


	// switch/flip buffers
	s_waterFlip ^= 1;
}

