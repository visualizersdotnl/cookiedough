
// cookiedough -- old school 2x2+4x4 map interpolation blitters plus buffers to use (for heavier effects)

/*
	FIXME:
	- missing a pixel on the side and on the bottom
	- adding explicit bugs or 'artsy' blitters may look very cool
*/

#include "main.h"
#include "map-blitter.h"
#include "cspan.h"

using namespace FXMAP;

uint32_t *g_pFXFine = nullptr;
uint32_t *g_pFXCoarse = nullptr;

bool MapBlitter_Create()
{
	g_pFXFine = static_cast<uint32_t*>(mallocAligned(kFineBytes, kCacheLine));
	g_pFXCoarse = static_cast<uint32_t*>(mallocAligned(kCoarseBytes, kCacheLine));

	return true;
}

void MapBlitter_Destroy()
{
	freeAligned(g_pFXFine);
	freeAligned(g_pFXCoarse);
}

// 2x2 blit (2 pixels per SSE write, uses 64-bit only intrinsic (FIXME?))
void MapBlitter_Colors_2x2(uint32_t* pDest, uint32_t* pSrc)
{
	VIZ_ASSERT(kFineDiv == 2);

	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi32(65536*kFineDiv);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY-1; ++iY)
	{
		for (int iX = 0; iX < kFineResX-1; ++iX)
		{
			const unsigned iA = iY*kFineResX + iX;
			const unsigned iB = iA+1;
			const unsigned iC = iA+kFineResX;
			const unsigned iD = iC+1;

			__m128i colA = c2vISSE32(pSrc[iA]);
			__m128i colB = c2vISSE32(pSrc[iB]);
			__m128i colC = c2vISSE32(pSrc[iC]);
			__m128i colD = c2vISSE32(pSrc[iD]);

			__m128i stepL = _mm_mul_epi32(_mm_sub_epi32(colC, colA), divisor);
			__m128i stepR = _mm_mul_epi32(_mm_sub_epi32(colD, colB), divisor);

			__m128i fpA = _mm_slli_epi32(colA, 16);
			__m128i fpB = _mm_slli_epi32(colB, 16);

			auto destIndex = (iY<<1)*kResX + (iX<<1);
			destIndex >>= 1;

			uint64_t *pCopy = reinterpret_cast<uint64_t*>(pDest);
			
			__m128i AB, CD;

			{
				__m128i delta = _mm_sub_epi32(fpB, fpA);
				delta = _mm_srli_epi32(delta, 16);
				__m128i step = _mm_madd_epi16(delta, divisor);
				__m128i color = fpA;
				__m128i pixel1 = _mm_srli_epi32(color, 16); 
				color = _mm_add_epi32(color, step);
				__m128i pixel2 = _mm_srli_epi32(color, 16); 
				AB = _mm_packus_epi32(pixel1, pixel2);
				pCopy[destIndex] = _mm_cvtsi128_si64(_mm_packus_epi16(AB, zero));
			}

			destIndex += kResX>>1;

			fpA = _mm_add_epi32(fpA, stepL);
			fpB = _mm_add_epi32(fpB, stepR);

			{
				__m128i delta = _mm_sub_epi32(fpB, fpA);
				delta = _mm_srli_epi32(delta, 16);
				__m128i step = _mm_madd_epi16(delta, divisor);
				__m128i color = fpA;
				__m128i pixel1 = _mm_srli_epi32(color, 16); 
				color = _mm_add_epi32(color, step);
				__m128i pixel2 = _mm_srli_epi32(color, 16); 
				CD = _mm_packus_epi32(pixel1, pixel2);
				pCopy[destIndex] = _mm_cvtsi128_si64(_mm_packus_epi16(CD, zero));
			}
		}
	}
}

// "interlaced" (read: intentionally broken) version of MapBlitter_Colors_2x2()
void MapBlitter_Colors_2x2_interlaced(uint32_t* pDest, uint32_t* pSrc)
{
	VIZ_ASSERT(kFineDiv == 2);

	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi32(65536*kFineDiv);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kFineResY-1; ++iY)
	{
		for (int iX = 0; iX < kFineResX-1; ++iX)
		{
			const unsigned iA = iY*kFineResX + iX;
			const unsigned iB = iA+1;
			const unsigned iC = iA+kFineResX;
			const unsigned iD = iC+1;

			__m128i colA = c2vISSE32(pSrc[iA]);
			__m128i colB = c2vISSE32(pSrc[iD]);

			__m128i fpA = _mm_slli_epi32(colA, 16);
			__m128i fpB = _mm_slli_epi32(colB, 16);

			auto destIndex = (iY<<1)*kResX + (iX<<1);
			destIndex >>= 1;

			uint64_t *pCopy = reinterpret_cast<uint64_t*>(pDest);

			{
				__m128i delta = _mm_sub_epi32(fpB, fpA);
				delta = _mm_srli_epi32(delta, 16);
				__m128i step = _mm_madd_epi16(delta, divisor);
				__m128i color = fpA;
				__m128i pixel1 = _mm_srli_epi32(color, 16); 
				color = _mm_add_epi32(color, step);
				__m128i pixel2 = _mm_srli_epi32(color, 16); 
				__m128i AB = _mm_packus_epi32(pixel1, pixel2);
				pCopy[destIndex] = _mm_cvtsi128_si64(_mm_packus_epi16(AB, zero));
			}

			destIndex += kResX>>1;

			{
				pCopy[destIndex] = pSrc[iC]; 
			}
		}
	}
}

// 4x4 blit (quasi-optimal use of ISSE)
// FIXME: won't optimize like the 2x2 version since it's hardly, if ever, used
void MapBlitter_Colors_4x4(uint32_t* pDest, uint32_t* pSrc) 
{
	VIZ_ASSERT(kCoarseDiv == 4);

	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi16(32768/kCoarseDiv);

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < kCoarseResY-1; ++iY)
	{
		for (int iX = 0; iX < kCoarseResX-1; ++iX)
		{
			const unsigned iA = iY*kCoarseResX + iX;
			const unsigned iB = iA+1;
			const unsigned iC = iA+kCoarseResX;
			const unsigned iD = iC+1;

			__m128i colA = c2vISSE(pSrc[iA]);
			__m128i colB = c2vISSE(pSrc[iB]);
			__m128i colC = c2vISSE(pSrc[iC]);
			__m128i colD = c2vISSE(pSrc[iD]);

			__m128i stepL = _mm_madd_epi16(_mm_unpacklo_epi16(_mm_sub_epi16(colC, colA), zero), divisor);
			__m128i stepR = _mm_madd_epi16(_mm_unpacklo_epi16(_mm_sub_epi16(colD, colB), zero), divisor);

			__m128i fpA = _mm_unpacklo_epi16(colA, zero);
			fpA = _mm_slli_epi32(fpA, 15);

			__m128i fpB = _mm_unpacklo_epi16(colB, zero);
			fpB = _mm_slli_epi32(fpB, 15);

			auto destIndex = (iY<<2)*kResX + (iX<<2);
			destIndex >>= 2;
			__m128i *pCopy = reinterpret_cast<__m128i*>(pDest);

			for (int blockY = 0; blockY < 4; ++blockY)
			{
				// if (blockY & 1)
				{
					__m128i delta = _mm_sub_epi32(fpB, fpA);
					delta = _mm_srli_epi32(delta, 15);
					__m128i step = _mm_madd_epi16(delta, divisor);
					__m128i color = fpA;
					__m128i pixel1 = _mm_srli_epi32(color, 15); 
					color = _mm_add_epi32(color, step);
					__m128i pixel2 = _mm_srli_epi32(color, 15); 
					color = _mm_add_epi32(color, step);
					__m128i AB = _mm_packs_epi32(pixel1, pixel2);
					__m128i pixel3 = _mm_srli_epi32(color, 15); 
					color = _mm_add_epi32(color, step);
					__m128i pixel4 = _mm_srli_epi32(color, 15);
					__m128i CD = _mm_packs_epi32(pixel3, pixel4);
					pCopy[destIndex] = _mm_packus_epi16(AB, CD);
				}

				fpA = _mm_add_epi32(fpA, stepL);
				fpB = _mm_add_epi32(fpB, stepR);
				destIndex += kResX>>2;
			}
		}
	}
}

#if 0 /* single channel reference test */

VIZ_INLINE int Monotone(uint32_t color)
{
	int R = color>>16 & 255;
	int G = color>>8  & 255;
	int B = color>>0  & 255;
	return (R+G+B)/3;
}

VIZ_INLINE int Mono_Delta4(uint32_t A, uint32_t B)
{
	unsigned monoA = Monotone(A);
	unsigned monoB = Monotone(B);
	return (monoB-monoA);
}

static void MapBlitter_Colors_Mono_Ref(uint32_t* pDest, uint32_t* pSrc)
{
	unsigned yIndex = 0;
	for (int iY = 0; iY < kFXMapResY-1; ++iY)
	{
		for (int iX = 0; iX < kFXMapResX-1; ++iX)
		{
			const unsigned iA = yIndex+iX;
			const unsigned iB = iA+1;
			const unsigned iC = iA+kFXMapResX;
			const unsigned iD = iC+1;

			int startX = Monotone(pSrc[iA])<<15;
			int dStartX = Mono_Delta4(pSrc[iA], pSrc[iC])*8192;

			int endX= Monotone(pSrc[iB])<<15;
			int dEndX = Mono_Delta4(pSrc[iB], pSrc[iD])*8192;

			uint32_t *pCopy = pDest;
			for (int blockY = 0; blockY < 4; ++blockY)
			{
				int colA = startX>>15;
				int colB = endX>>15;
				cspanISSE_noclip_4(pCopy, c2vISSE(colA*0x010101), c2vISSE(colB*0x010101));
				pCopy  += kResX;
				startX += dStartX;
				endX   += dEndX;
			}

			pDest += 4;
		}

		pDest  += 3*kResX + 4;
		yIndex += kFXMapResX;
	}
}

#endif
