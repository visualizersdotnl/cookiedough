
// cookiedough -- old school 2x2+4x4 map bilinear blitters plus buffers to use (for heavier effects)

#include "main.h"
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

// 2x2 blit (2 pixels per SSE write)
void Fx_Blit_2x2(uint32_t* pDest, const uint32_t* pSrc)
{
	VIZ_ASSERT_ALIGNED(pSrc);
	VIZ_ASSERT_ALIGNED(pDest);

	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi32(65536/kFxMapDiv);

	int64_t *pCopy = reinterpret_cast<int64_t*>(pDest);

	#pragma omp parallel for schedule(static)
	for (unsigned iY = 0; iY < kFxMapResY-4; ++iY)
	{
		const unsigned mapIndexY = iY*kFxMapResX;
		const unsigned destIndexY = (iY*kFxMapDiv)*kResX;

		for (unsigned iX = 0; iX < kFxMapResX-4; ++iX)
		{
			const unsigned iA = mapIndexY + iX;
//			const unsigned iB = iA+1;
			const unsigned iC = iA+kFxMapResX;
//			const unsigned iD = iC+1;

			auto destIndex = destIndexY + (iX*kFxMapDiv);
			destIndex >>= 1;

			// FIXME: optimize SIMD

			const __m128i colA32 = c2vISSE32(pSrc[iA]);
			const __m128i colB32 = c2vISSE32(pSrc[iA+1]);
			const __m128i colC32 = c2vISSE32(pSrc[iC]);
			const __m128i colD32 = c2vISSE32(pSrc[iC+1]);

			const __m128i dX = _mm_madd_epi16(_mm_sub_epi32(colB32, colA32), divisor);
			const __m128i dY = _mm_madd_epi16(_mm_sub_epi32(colD32, colC32), divisor);

			__m128i X = _mm_slli_epi32(colA32, 16);
			__m128i Y = _mm_slli_epi32(colC32, 16);

			for (int iPixel = 0; iPixel < 2; ++iPixel)
			{
				const __m128i dA = _mm_madd_epi16(_mm_srli_epi32(_mm_sub_epi32(Y, X), 16), divisor);
				const __m128i A = _mm_srli_epi32(X, 16); 
				const __m128i B = _mm_srli_epi32(_mm_add_epi32(A, dA), 16); 
				const __m128i AB = _mm_packus_epi32(A, B);
				const __m128i C = _mm_packus_epi16(AB, zero);

				_mm_stream_si64(pCopy+destIndex, _mm_cvtsi128_si64(C));				
				destIndex += kResX>>1;

				X = _mm_add_epi32(X, dX);
				Y = _mm_add_epi32(Y, dY);
			}
		}
	}

	CKD_FLANDERS(_mm_sfence();)
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


