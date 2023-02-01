
// cookiedough -- basic utilities

#include "main.h"
// #include "util.h"

#if !_WIN64 && defined(FOR_INTEL)

// memcpy_fast()
// - approx. twice faster than it's CRT counterpart on a 2009 netbook with 1.66GHz Intel Atom CPU
// - combined writes (MOVNTQ) perform favorably for intended usage pattern

void memcpy_fast(void *pDest, const void *pSrc, size_t numBytes)
{
	// numBytes must be a multiple of 64 -- use memcpy() for general purpose
	VIZ_ASSERT(!(numBytes & 63));
	
	// an 8-byte boundary gaurantees correctly aligned reads and writes
	VIZ_ASSERT( 0 ==  (int(pSrc) & 7) );
	VIZ_ASSERT( 0 == (int(pDest) & 7) );
	
	__asm
	{
		mov    edi, pDest
		mov    esi, pSrc
		mov    ecx, numBytes
		shr    ecx, 6
	_loop:
		movq   mm1, [esi]
		movq   mm2, [esi+8]
		movq   mm3, [esi+16]
		movq   mm4, [esi+24]
		movq   mm5, [esi+32]
		movq   mm6, [esi+40]
		movq   mm7, [esi+48]
		movq   mm0, [esi+56]
		movntq [edi], mm1
		movntq [edi+8], mm2
		movntq [edi+16], mm3
		movntq [edi+24], mm4
		movntq [edi+32], mm5
		movntq [edi+40], mm6
		movntq [edi+48], mm7
		movntq [edi+56], mm0
		add    esi, 64
		add    edi, 64
		dec    ecx
		jnz    _loop
	}

	_mm_empty();
}

/*
void memset32(void *pDest, uint32_t value, size_t numInts)
{
	// must be a multiple of 16 bytes -- use memset() for general purpose
	VIZ_ASSERT(!(numInts & 3));

	// an 8-byte boundary gaurantees correctly aligned writes
	VIZ_ASSERT(!(reinterpret_cast<uint32_t>(pDest) & 7));

	__asm
	{
		mov       edi, pDest
		movq      mm0, value
		punpckldq mm0, mm0
		mov       ecx, numInts
		shr       ecx, 2
	_loop:
		movntq    [edi], mm0
		movntq    [edi+8], mm0
		add       edi, 16
		dec       ecx
		jnz       _loop
	}		

	_mm_empty();
}
*/

#endif

void Mix32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels, uint8_t alpha)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i alphaUnp = _mm_unpacklo_epi8(_mm_cvtsi32_si128(0x01010101 * alpha), zero);
	for (unsigned int iPixel = 0; iPixel < numPixels; ++iPixel)
	{
		const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pSrc[iPixel]), zero);
		const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iPixel]), zero);
		const __m128i delta = _mm_mullo_epi16(alphaUnp, _mm_sub_epi16(srcColor, destColor));
		const __m128i color = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(destColor, 8), delta), 8);
		pDest[iPixel] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
	}

#if 0
	uint32_t aPacked;
	aPacked = alpha * 0x01010101;

	__asm
	{
		xor         eax, eax
		mov         edi, pDest
		mov         esi, pSrc
		mov         ecx, numPixels
		movd        mm1, aPacked
		pxor        mm0, mm0
		punpcklbw   mm1, mm0
	_loop:
		movd        mm3, [esi+eax]
		movd        mm4, [edi+eax]
		punpcklbw   mm3, mm0
		punpcklbw   mm4, mm0
		psubw       mm3, mm4
		psllw       mm4, 8
		pmullw      mm3, mm1
		paddw       mm3, mm4
		psrlw       mm3, 8
		packuswb    mm3, mm0
		movd        [edi+eax], mm3
		add         eax, 4
		dec         ecx
		jnz         _loop
	}

	_mm_empty();
#endif
}

void Add32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels)
{
	const __m128i zero = _mm_setzero_si128();
	for (unsigned int iPixel = 0; iPixel < numPixels; ++iPixel)
	{
		const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pSrc[iPixel]), zero);
		const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iPixel]), zero);
		const __m128i delta = srcColor;
		const __m128i color = _mm_add_epi16(destColor, delta);
		pDest[iPixel] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
	}
}

void MixSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels)
{
	const __m128i zero = _mm_setzero_si128();
	for (unsigned int iPixel = 0; iPixel < numPixels; ++iPixel)
	{
		const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pSrc[iPixel]), zero);
		const __m128i alphaUnp = _mm_shufflelo_epi16(srcColor, 0xff);
		const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iPixel]), zero);
		const __m128i delta = _mm_mullo_epi16(alphaUnp, _mm_sub_epi16(srcColor, destColor));
		const __m128i color = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(destColor, 8), delta), 8);
		pDest[iPixel] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
	}

#if 0
	__asm
	{
		xor        eax, eax
		mov        edi, pDest
		mov        esi, pSrc
		mov        ecx, numPixels
		pxor       mm0, mm0
	_loop:
		movd       mm3, [esi+eax]
		movd       mm4, [edi+eax]
		punpcklbw  mm3, mm0
		punpcklbw  mm4, mm0
 		pshufw     mm1, mm3, 0xff
 		psubw      mm3, mm4
		psllw      mm4, 8
 		pmullw     mm3, mm1
		paddw      mm3, mm4
		psrlw      mm3, 8
		packuswb   mm3, mm0
		movd       [edi+eax], mm3
		add        eax, 4
		dec        ecx
		jnz        _loop
	}

	_mm_empty();
#endif
}

// FIXME: optimize
void BlitSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes)
{
	const __m128i zero = _mm_setzero_si128();

	#pragma omp parallel for schedule(dynamic)
	for (int iY = 0; iY < int(yRes); ++iY)
	{
		const uint32_t *srcPixel = pSrc + iY*srcResX;
		uint32_t *destPixel = pDest + iY*destResX;

		for (unsigned iX = 0; iX < srcResX; ++iX)
		{
			const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*srcPixel++), zero);
			const __m128i alphaUnp = _mm_shufflelo_epi16(srcColor, 0xff);
			const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*destPixel), zero);
			const __m128i delta = _mm_mullo_epi16(alphaUnp, _mm_sub_epi16(srcColor, destColor));
			const __m128i color = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(destColor, 8), delta), 8);
			*destPixel++ = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
		}
	}
}

void BlitSrc32A(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes, float alpha)
{
	VIZ_ASSERT(alpha >= 0.f && alpha <= 255.f);

	const __m128i zero = _mm_setzero_si128();
	const __m128i fixedAlphaUnp = c2vISSE16(0x01010101 * unsigned(alpha*255.f));

	for (int iY = 0; iY < int(yRes); ++iY)
	{
		for (unsigned iX = 0; iX < srcResX; ++iX)
		{
			const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pSrc[iX]), zero);
			const __m128i alphaUnp = _mm_srli_epi16(_mm_mullo_epi16(_mm_shufflelo_epi16(srcColor, 0xff), fixedAlphaUnp), 8);
			const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iX]), zero);
			const __m128i delta = _mm_mullo_epi16(alphaUnp, _mm_sub_epi16(srcColor, destColor));
			const __m128i color = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(destColor, 8), delta), 8);
			pDest[iX] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
		}

		pSrc += srcResX;
		pDest += destResX;
	}
}

void BlitAdd32(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes)
{
	const __m128i zero = _mm_setzero_si128();

	#pragma omp parallel for schedule(dynamic)
	for (int iY = 0; iY < int(yRes); ++iY)
	{
		// FIXME: if getting rid of OMP, move these out of the loop and just increment them
		const uint32_t *srcPixel = pSrc + iY*srcResX;
		uint32_t *destPixel = pDest + iY*destResX;

		for (unsigned iX = 0; iX < srcResX; ++iX)
		{
			const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(srcPixel[iX]), zero);
			const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(destPixel[iX]), zero);
			const __m128i delta = srcColor;
			const __m128i color = _mm_add_epi16(destColor, delta);
			destPixel[iX] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
		}
	}
}

void Fade32(uint32_t *pDest, unsigned int numPixels, uint32_t RGB, uint8_t alpha)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i alphaUnp = _mm_unpacklo_epi8(_mm_cvtsi32_si128(0x01010101 * alpha), zero);
	const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(RGB), zero);
	for (unsigned int iPixel = 0; iPixel < numPixels; ++iPixel)
	{
		const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iPixel]), zero);
		const __m128i delta = _mm_mullo_epi16(alphaUnp, _mm_sub_epi16(srcColor, destColor));
		const __m128i color = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(destColor, 8), delta), 8);
		pDest[iPixel] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
	}
}
