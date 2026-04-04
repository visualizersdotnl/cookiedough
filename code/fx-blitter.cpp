
// cookiedough -- old school 2x2 map bilinear blitters plus buffers to use (for heavier effects)

#include "main.h"
#include "util.h"
#include "fx-blitter.h"

uint32_t *g_pFxMap[kNumFxMaps] = { nullptr };

bool FxBlitter_Create()
{
	uint32_t* basePtr = static_cast<uint32_t*>(mallocAligned(kFxMapBytes*4, kAlignTo));

	g_pFxMap[0] = basePtr;
	g_pFxMap[1] = g_pFxMap[0] + kFxMapSize;
	g_pFxMap[2] = g_pFxMap[1] + kFxMapSize;
	g_pFxMap[3] = g_pFxMap[2] + kFxMapSize;

	return true;
}

void FxBlitter_Destroy()
{
	freeAligned(g_pFxMap[0]);
}

void Fx_Blit_2x2(uint32_t* pDest, const uint32_t* pSrc)
{
	VIZ_ASSERT_ALIGNED(pDest);
	VIZ_ASSERT_ALIGNED(pSrc);

	#pragma omp parallel for schedule(static)
	for (unsigned iY = 0; iY < kFxMapResY-4; ++iY)
	{
		const __m128i *pSrcRow0 = reinterpret_cast<const __m128i*>(&pSrc[iY*kFxMapResX]);
		const __m128i *pSrcRow1 = reinterpret_cast<const __m128i*>(&pSrc[(iY+1)*kFxMapResX]);

		__m128i* pDstTop = reinterpret_cast<__m128i*>(&pDest[(iY<<1)*kResX]);
		__m128i* pDstBot = reinterpret_cast<__m128i*>(&pDest[((iY<<1)+1)*kResX]);

		for (unsigned iX = 0; iX < (kFxMapResX-4)/4; ++iX)
		{
			// load quad pixels
			const __m128i r0c0 = _mm_load_si128(pSrcRow0+iX); 
			const __m128i r1c0 = _mm_load_si128(pSrcRow1+iX);
			
			// fetch next 4 pixels to get right hand neighbours for interpolation (this is where the guard band allows for a full extra 128-bit load)
			const __m128i r0c1 = _mm_load_si128(reinterpret_cast<const __m128i*>(&pSrc[iY*kFxMapResX + (iX<<2) + 1]));
			const __m128i r1c1 = _mm_load_si128(reinterpret_cast<const __m128i*>(&pSrc[(iY+1)*kFxMapResX + (iX<<2) + 1]));

			// avg. horz. top/bottom rows
			const __m128i avgH0 = _mm_avg_epu8(r0c0, r0c1);
			const __m128i avgH1 = _mm_avg_epu8(r1c0, r1c1);

			// vertical avg.
			const __m128i avgV0 = _mm_avg_epu8(r0c0, r1c0);
			const __m128i avgCenter = _mm_avg_epu8(avgH0, avgH1);

			// unpack/interleave into dest. pairs
			const __m128i dstTopL = _mm_unpacklo_epi32(r0c0, avgH0);
			const __m128i dstTopR = _mm_unpackhi_epi32(r0c0, avgH0);
			
			const __m128i dstBotL = _mm_unpacklo_epi32(avgV0, avgCenter);
			const __m128i dstBotR = _mm_unpackhi_epi32(avgV0, avgCenter);

			// store 8 pixels per row (4 x 128-bit stores)
			_mm_store_si128(pDstTop + (iX<<1),  dstTopL);
			_mm_store_si128(pDstTop + (iX<<1)+1,dstTopR);
			_mm_store_si128(pDstBot + (iX<<1),  dstBotL);
			_mm_store_si128(pDstBot + (iX<<1)+1,dstBotR);
		}
	}
 
	CKD_FLANDERS(_mm_sfence());
}

void FxBlitter_DrawTestPattern(uint32_t* pDest)
{
	for (unsigned iY = 0; iY < kFxMapResY; ++iY)
	{
		for (unsigned iX = 0; iX < kFxMapResX; ++iX)
		{
			int color;
			if (iY < kFxMapResY / 2)
			{
				color = (iY&1) ? -1 : 0;
			}
			else
			{
				color = (iX&1) ? -1 : 0;
			}

			g_pFxMap[0][iY*kFxMapResX + iX] = color;
		}
	}

	Fx_Blit_2x2(pDest, g_pFxMap[0]);
}


