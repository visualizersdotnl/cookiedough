
// cookiedough -- bilinear texture samplers (8-bit & 32-bit)

#ifndef _BILINEAR_H_
#define _BILINEAR_H_

// derive required sampling parameters from 24:8 fixed point UV:
// - U and V can be signed, as long as they have the 8-bit fractional part
// - target texture(s) expected to have an equal power-of-2 dimension
VIZ_INLINE void bsamp_prepUVs(
	int U, int V,
	unsigned int mapAnd, unsigned int mapShift,
	unsigned int &U0, unsigned int &V0,
	unsigned int &U1, unsigned int &V1,
	unsigned int &fracU, unsigned int &fracV)
{
	U0 = U >> 8;
	V0 = V >> 8;
	U1 = U0 + 1;
	V1 = V0 + 1;
	U0 &= mapAnd;
	V0 = (V0 & mapAnd) << mapShift;
	U1 &= mapAnd;
	V1 = (V1 & mapAnd) << mapShift;
	fracU = U & 0xff;
	fracV = V & 0xff;

	// distribute across each byte (aids bsamp32_16())
	fracU *= 0x01010101;
	fracV *= 0x01010101;
}

// sample 8-bit texture
VIZ_INLINE unsigned int bsamp8(
	const uint8_t *pTexture, 
	unsigned int U0, unsigned int V0, 
	unsigned int U1, unsigned int V1, 
	unsigned int fracU, unsigned int fracV)
{
	// forcing signed integers here to keep the arithmetic "correct"
	// in practice though, for x86, only the shift instructions differ (arithmetic, not logical)
	const int S0 = pTexture[U0+V0];
	const int S1 = pTexture[U1+V0];
	const int S2 = pTexture[U0+V1];
	const int S3 = pTexture[U1+V1];
	const int dX1 = (S1-S0)*signed(fracU&0xff);
	const int dX2 = (S3-S2)*signed(fracU&0xff);
	const int S01 = ((S0<<8)+dX1)>>8;
	const int S23 = ((S2<<8)+dX2)>>8;
	const int dY = (S23-S01)*signed(fracV&0xff);
	return ((S01<<8)+dY)>>8;
}

// sample 32-bit texture
// color is returned as (unpacked) 16-bit ISSE vector
// FIXME: I think I can take a few instructions off here by interleaving
VIZ_INLINE __m128i bsamp32_16(
	const uint32_t *pTexture, 
	unsigned int U0, unsigned int V0, 
	unsigned int U1, unsigned int V1, 
	unsigned int fracU, unsigned int fracV)
{
	const __m128i zero = _mm_setzero_si128();
	__m128i _fracU = _mm_cvtsi32_si128(fracU);
	__m128i _fracV = _mm_cvtsi32_si128(fracV);
	_fracU = _mm_unpacklo_epi8(_fracU, zero);
	_fracV = _mm_unpacklo_epi8(_fracV, zero);
	__m128i S0 = _mm_cvtsi32_si128(pTexture[U0+V0]);
	__m128i S1 = _mm_cvtsi32_si128(pTexture[U1+V0]);
	__m128i S2 = _mm_cvtsi32_si128(pTexture[U0+V1]);
	__m128i S3 = _mm_cvtsi32_si128(pTexture[U1+V1]);
	S0 = _mm_unpacklo_epi8(S0, zero);
	S1 = _mm_unpacklo_epi8(S1, zero);
	S2 = _mm_unpacklo_epi8(S2, zero);
	S3 = _mm_unpacklo_epi8(S3, zero);
	const __m128i dX1 = _mm_mullo_epi16(_mm_sub_epi16(S1, S0), _fracU);
	const __m128i dX2 = _mm_mullo_epi16(_mm_sub_epi16(S3, S2), _fracU);
	const __m128i S01 = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(S0, 8), dX1), 8);
	const __m128i S23 = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(S2, 8), dX2), 8);
	const __m128i dY = _mm_mullo_epi16(_mm_sub_epi16(S23, S01), _fracV);
	return _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(S01, 8), dY), 8);
}

// same as bsamp32_16(), but slightly optimized and returns a 32-bit ISSE vector
VIZ_INLINE __m128i bsamp32_32(
	const uint32_t *pTexture, 
	unsigned int U0, unsigned int V0, 
	unsigned int U1, unsigned int V1, 
	unsigned int fracU, unsigned int fracV)
{
	VIZ_ASSERT(false); // implement!
	return _mm_setzero_si128();
}

#endif // _BILINEAR_H_
