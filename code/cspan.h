
// cookiedough -- cspan() & friends: colored span draw functions

#ifndef _CSPAN_H_
#define _CSPAN_H_

// draw span (straight line) with interpolated color (ISSE-optimized)
VIZ_INLINE void cspan(
	uint32_t *pDest, 
	int destIncr, 
	unsigned int length,     // length of span
	unsigned int drawLength, // visible length (correct interpolation along occluded part!)
	uint32_t A, uint32_t B)
{
	VIZ_ASSERT(drawLength > 0 && length >= drawLength);

	// minor trick ahead:
	// to calculate the step values using PMADDWD, the 16-bit deltas are unpacked to 32-bit
	// this effectively aligns each 16-bit delta and keeps the uneven 16-bit values zero

	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi16(32768 / length);
	__m128i from = _mm_unpacklo_epi8(_mm_cvtsi32_si128(A), zero);
	const __m128i to =  _mm_unpacklo_epi8(_mm_cvtsi32_si128(B), zero);
	const __m128i delta = _mm_unpacklo_epi16(_mm_sub_epi16(to, from), zero);
	const unsigned int preSteps = length - drawLength;
	const __m128i preStep = _mm_madd_epi16(delta, _mm_mullo_epi16(divisor, _mm_set1_epi16(preSteps)));
	const __m128i step = _mm_madd_epi16(delta, divisor);
	from = _mm_unpacklo_epi16(from, zero);
	from = _mm_slli_epi32(from, 15);
	from = _mm_add_epi32(from, preStep);

	while (drawLength--)
	{
		__m128i color = _mm_srli_epi32(from, 15);
		color = _mm_packs_epi32(color, zero);
		color = _mm_packus_epi16(color, zero);
		*pDest = _mm_cvtsi128_si32(color);
		pDest += destIncr;
		from = _mm_add_epi32(from, step);
	}
}

#if 0

// copy of cspan(), takes pre-unpacked colors as __m128i
// for better integration with ISSE-optimized caller
VIZ_INLINE void cspanISSE(
	uint32_t *pDest,
	int destIncr, 
	unsigned int length,     // length of span
	unsigned int drawLength, // visible length (for correct interpolation)
	__m128i A, __m128i B)    // colors, unpacked to 16-bit
{
	VIZ_ASSERT(drawLength > 0 && length >= drawLength);

	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi16(32768 / length);
	const __m128i delta = _mm_unpacklo_epi16(_mm_sub_epi16(B, A), zero);
	const unsigned int preSteps = length - drawLength;
	const __m128i preStep = _mm_madd_epi16(delta, _mm_mullo_epi16(divisor, _mm_set1_epi16(preSteps)));
	const __m128i step = _mm_madd_epi16(delta, divisor);
	A = _mm_unpacklo_epi16(A, zero);
	A = _mm_slli_epi32(A, 15);
	A = _mm_add_epi32(A, preStep);

	while (drawLength--)
	{
		__m128i color = _mm_srli_epi32(A, 15);
		color = _mm_packs_epi32(color, zero);
		color = _mm_packus_epi16(color, zero);
		*pDest = _mm_cvtsi128_si32(color);
		pDest += destIncr;
		A = _mm_add_epi32(A, step);
	}
}

#else // FIXME: temp. version with slightly better precision

// copy of cspan(), takes pre-unpacked colors as __m128i
// for better integration with ISSE-optimized caller
VIZ_INLINE void cspanISSE(
	uint32_t *pDest,
	int destIncr, 
	unsigned int length,     // length of span
	unsigned int drawLength, // visible length (for correct interpolation)
	__m128i A, __m128i B)    // colors, unpacked to 16-bit
{
	VIZ_ASSERT(drawLength > 0 && length >= drawLength);

	const __m128i zero = _mm_setzero_si128();

	A = _mm_unpacklo_epi16(A, zero);
	B = _mm_unpacklo_epi16(B, zero);

	const __m128i divisor = _mm_set1_epi32(65536/length);
	const __m128i delta = _mm_sub_epi32(B, A);

	const unsigned int preSteps = length - drawLength;
	const __m128i preStep = _mm_madd_epi16(delta, _mm_mul_epi32(divisor, _mm_set1_epi32(preSteps)));
	const __m128i step = _mm_madd_epi16(delta, divisor);

	A = _mm_slli_epi32(A, 16);
	A = _mm_add_epi32(A, preStep);

	while (drawLength--)
	{
		__m128i color = _mm_srli_epi32(A, 16);
		color = _mm_packus_epi32(color, zero);
		color = _mm_packus_epi16(color, zero);
		*pDest = _mm_cvtsi128_si32(color);
		pDest += destIncr;
		A = _mm_add_epi32(A, step);
	}
}

#endif

// copy of cspanISSE() without clipping (slightly faster)
VIZ_INLINE void cspanISSE_noclip(
	uint32_t *pDest,
	int destIncr, 
	unsigned int length,  // length of span
	__m128i A, __m128i B) // colors, unpacked to 16-bit	
{
	VIZ_ASSERT(length > 0);

	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi16(32768 / length);
	const __m128i delta = _mm_unpacklo_epi16(_mm_sub_epi16(B, A), zero);
	const __m128i step = _mm_madd_epi16(delta, divisor);
	A = _mm_unpacklo_epi16(A, zero);
	A = _mm_slli_epi32(A, 15);

	while (length--)
	{
		__m128i color = _mm_srli_epi32(A, 15);
		color = _mm_packs_epi32(color, zero);
		color = _mm_packus_epi16(color, zero);
		*pDest = _mm_cvtsi128_si32(color);
		pDest += destIncr;
		A = _mm_add_epi32(A, step);
	}
}

// copy of cspanISSE_noclip() that handles only 4 horizontal pixels
VIZ_INLINE void cspanISSE_noclip_4(
	uint32_t *pDest,
	__m128i A, __m128i B) // colors, unpacked to 16-bit	
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i divisor = _mm_set1_epi16(8192);
	const __m128i delta = _mm_unpacklo_epi16(_mm_sub_epi16(B, A), zero);
	const __m128i step = _mm_madd_epi16(delta, divisor);
	A = _mm_unpacklo_epi16(A, zero);
	A = _mm_slli_epi32(A, 15);
		
	const __m128i pixel1 = _mm_srli_epi32(A, 15); 
	A = _mm_add_epi32(A, step);
	const __m128i pixel2 = _mm_srli_epi32(A, 15); 
	A = _mm_add_epi32(A, step);
	const __m128i AB = _mm_packs_epi32(pixel1, pixel2);
	const __m128i pixel3 = _mm_srli_epi32(A, 15); 
	A = _mm_add_epi32(A, step);
	const __m128i pixel4 = _mm_srli_epi32(A, 15);
	const __m128i CD = _mm_packs_epi32(pixel3, pixel4);
	*reinterpret_cast<__m128i*>(pDest) = _mm_packus_epi16(AB, CD);
}

#endif // _CSPAN_H_
