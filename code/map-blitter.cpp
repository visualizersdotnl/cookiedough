
// cookiedough -- old school 4x4 map interpolation blitters (mainly for computationally heavy effects)

/*
	- we're missing a pixel on the side and on the bottom
	- adding explicit bugs or 'artsy' blitters may look very cool
*/

#include "main.h"
#include "map-blitter.h"
#include "cspan.h"

bool MapBlitter_Create()
{
	return true;
}

void MapBlitter_Destroy()
{

}

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

// FIXME: 
// - try OpenMP
// - use sliding window
void MapBlitter_Colors(uint32_t* pDest, uint32_t* pSrc)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi16(32768/kFXMapDiv);

	unsigned yIndex = 0;
	for (int iY = 0; iY < kFXMapResY-1; ++iY)
	{
		for (int iX = 0; iX < kFXMapResX-1; ++iX)
		{
			const unsigned iA = yIndex+iX;
			const unsigned iB = iA+1;
			const unsigned iC = iA+kFXMapResX;
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

			__m128i *pCopy = reinterpret_cast<__m128i*>(pDest);
			for (int blockY = 0; blockY < 4; ++blockY)
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
				*reinterpret_cast<__m128i*>(pCopy) = _mm_packus_epi16(AB, CD);
				fpA = _mm_add_epi32(fpA, stepL);
				fpB = _mm_add_epi32(fpB, stepR);
				pCopy += kResX>>2;
			}

			pDest += 4;
		}

		pDest  += 3*kResX + 4;
		yIndex += kFXMapResX;
	}
}

/*
void MapBlitter_UV(...)
{
}
*/
