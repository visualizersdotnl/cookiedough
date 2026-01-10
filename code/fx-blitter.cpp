
// cookiedough -- old school 2x2+4x4 map interpolation blitters plus buffers to use (for heavier effects)

#include "main.h"
#include "fx-blitter.h"

uint32_t *g_pFxMap[4] = { nullptr };

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

// 2x2 blit (2 pixels per SSE write)
// FIXME: render to bigger map (X+1, Y+1) instead of copy hack below
void Fx_Blit_2x2(uint32_t* pDest, uint32_t* pSrc)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi32(65536/kFxMapDiv);

	uint64_t *pCopy = reinterpret_cast<uint64_t*>(pDest);

	#pragma omp parallel for schedule(static)
	for (unsigned iY = 0; iY < kFxMapResY-1; ++iY)
	{
		const unsigned mapIndexY = iY*kFxMapResX;
		const unsigned destIndexY = (iY*kFxMapDiv)*kResX;

		for (unsigned iX = 0; iX < kFxMapResX-1; ++iX)
		{
			const unsigned iA = mapIndexY + iX;
//			const unsigned iB = iA+1;
			const unsigned iC = iA+kFxMapResX;
//			const unsigned iD = iC+1;

			const __m128i colA = c2vISSE32(pSrc[iA]);
			const __m128i colB = c2vISSE32(pSrc[iA+1]);
			const __m128i colC = c2vISSE32(pSrc[iC]);
			const __m128i colD = c2vISSE32(pSrc[iC+1]);

			const __m128i stepY0 = _mm_madd_epi16(_mm_sub_epi32(colB, colA), divisor);
			const __m128i stepY1 = _mm_madd_epi16(_mm_sub_epi32(colD, colC), divisor);

			__m128i fromY0 = _mm_slli_epi32(colA, 16);
			__m128i fromY1 = _mm_slli_epi32(colC, 16);

			auto destIndex = destIndexY + (iX*kFxMapDiv);
			destIndex >>= 1;

			for (int iPixel = 0; iPixel < 2; ++iPixel)
			{
				__m128i step = _mm_madd_epi16(_mm_srli_epi32(_mm_sub_epi32(fromY1, fromY0), 16), divisor);
				__m128i color = fromY0;
				__m128i A = _mm_srli_epi32(color, 16); 
				__m128i B = _mm_srli_epi32(_mm_add_epi32(color, step), 16); 
				__m128i AB = _mm_packus_epi32(A, B);

				pCopy[destIndex] = _mm_cvtsi128_si64(_mm_packus_epi16(AB, zero));
				destIndex += kResX>>1;

				fromY0 = _mm_add_epi32(fromY0, stepY0);
				fromY1 = _mm_add_epi32(fromY1, stepY1);
			}
		}

		// Border: copy last 2 pixels for both dest. columns
		const auto destIndex = (destIndexY + ((kFxMapResX-2)*kFxMapDiv))>>1;
		pCopy[destIndex+1] = pCopy[destIndex];
		pCopy[destIndex+1+(kResX>>1)] = pCopy[destIndex+(kResX>>1)];
	}

	// Copy last 2 dest. rows
	auto srcIndex = (kResY-4)*kResX;
	auto destIndex = srcIndex + kResX*2;
	memcpy(pDest+destIndex, pDest+srcIndex, kResX*2*sizeof(uint32_t));
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
