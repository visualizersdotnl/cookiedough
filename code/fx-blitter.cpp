
// cookiedough -- old school 2x2+4x4 map interpolation blitters plus buffers to use (for heavier effects)

#include "main.h"
#include "fx-blitter.h"

uint32_t *g_pFxMap = nullptr;

bool FxBlitter_Create()
{
	g_pFxMap = static_cast<uint32_t*>(mallocAligned(kFxMapBytes, kCacheLine));

	return true;
}

void FxBlitter_Destroy()
{
	freeAligned(g_pFxMap);
}

// 2x2 blit (2 pixels per SSE write, uses 64-bit only intrinsic (potential FIXME)
void Fx_Blit_2x2(uint32_t* pDest, uint32_t* pSrc)
{
	VIZ_ASSERT(kFxMapDiv == 2);

	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi32(65536/kFxMapDiv);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY-1; ++iY)
	{
		for (int iX = 0; iX < kFxMapResX-1; ++iX)
		{
			const unsigned iA = iY*kFxMapResX + iX;
			const unsigned iB = iA+1;
			const unsigned iC = iA+kFxMapResX;
			const unsigned iD = iC+1;

			__m128i colA = c2vISSE32(pSrc[iA]);
			__m128i colB = c2vISSE32(pSrc[iB]);
			__m128i colC = c2vISSE32(pSrc[iC]);
			__m128i colD = c2vISSE32(pSrc[iD]);

			__m128i stepY0 = _mm_madd_epi16(_mm_sub_epi32(colB, colA), divisor);
			__m128i stepY1 = _mm_madd_epi16(_mm_sub_epi32(colD, colC), divisor);

			__m128i fromY0 = _mm_slli_epi32(colA, 16);
			__m128i fromY1 = _mm_slli_epi32(colC, 16);

			auto destIndex = (iY<<1)*kResX + (iX<<1);
			destIndex >>= 1;

			uint64_t *pCopy = reinterpret_cast<uint64_t*>(pDest);

			// FIXME: can save a few instructions by unrolling
			for (int iPixel = 0; iPixel < 2; ++iPixel)
			{
				__m128i step = _mm_madd_epi16(_mm_srli_epi32(_mm_sub_epi32(fromY1, fromY0), 16), divisor);
				__m128i color = fromY0;
				__m128i A = _mm_srli_epi32(color, 16); 
				__m128i B = _mm_srli_epi32(_mm_add_epi32(color, step), 16); 
				__m128i AB = _mm_packus_epi32(A, B);

				pCopy[destIndex] = _mm_cvtsi128_si64(_mm_packus_epi16(AB, zero));

				fromY0 = _mm_add_epi32(fromY0, stepY0);
				fromY1 = _mm_add_epi32(fromY1, stepY1);

				destIndex += kResX>>1;
			}
		}
	}
}

// basically just a broken version
void Fx_Blit_2x2_Dithered(uint32_t* pDest, uint32_t* pSrc)
{
	VIZ_ASSERT(kFxMapDiv == 2);

	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi32(65536/kFxMapDiv);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFxMapResY-1; ++iY)
	{
		for (int iX = 0; iX < kFxMapResX-1; ++iX)
		{
			const unsigned iA = iY*kFxMapResX + iX;
			const unsigned iB = iA+1;
			const unsigned iC = iA+kFxMapResX;
			const unsigned iD = iC+1;

			__m128i colA = c2vISSE32(pSrc[iA]);
			__m128i colB = c2vISSE32(pSrc[iD]);
			__m128i colC = c2vISSE32(0);
			__m128i colD = c2vISSE32(0);

			__m128i stepY0 = _mm_madd_epi16(_mm_sub_epi32(colB, colA), divisor);
			__m128i stepY1 = _mm_madd_epi16(_mm_sub_epi32(colD, colC), divisor);

			__m128i fromY0 = _mm_slli_epi32(colA, 16);
			__m128i fromY1 = _mm_slli_epi32(colC, 16);

			auto destIndex = (iY<<1)*kResX + (iX<<1);
			destIndex >>= 1;

			uint64_t *pCopy = reinterpret_cast<uint64_t*>(pDest);

			int iPixel = 0; // for (int iPixel = 0; iPixel < 2; ++iPixel)
			{
				__m128i step = _mm_madd_epi16(_mm_srli_epi32(_mm_sub_epi32(fromY1, fromY0), 16), divisor);
				__m128i color = fromY0;
				__m128i A = _mm_srli_epi32(color, 16); 
				__m128i B = _mm_srli_epi32(_mm_add_epi32(color, step), 16); 
				__m128i AB = _mm_packus_epi32(A, B);

				pCopy[destIndex] = _mm_cvtsi128_si64(_mm_packus_epi16(AB, zero));

				fromY0 = _mm_add_epi32(fromY0, stepY0);
				fromY1 = _mm_add_epi32(fromY1, stepY1);

				destIndex += kResX>>1;
			}
		}
	}
}

void FxBlitter_DrawTestPattern(uint32_t* pDest)
{
	for (int iY = 0; iY < kFxMapResY; ++iY)
	{
		for (int iX = 0; iX < kFxMapResX; ++iX)
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

			g_pFxMap[iY*kFxMapResX + iX] = color;
		}
	}

	Fx_Blit_2x2(pDest, g_pFxMap);
}
