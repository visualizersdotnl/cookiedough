
// cookiedough -- basic utilities

#include "main.h"
// #include "util.h"
#include "bilinear.h"

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

	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
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

	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
		const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pSrc[iPixel]), zero);
		const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iPixel]), zero);
		const __m128i delta = srcColor;
		const __m128i color = _mm_add_epi16(destColor, delta);
		pDest[iPixel] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
	}
}

// FIXME: optimize (SIMD)
void MixOver32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels)
{
	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
			const uint32_t destPixel = pDest[iPixel];
			const uint32_t srcPixel  = pSrc[iPixel];

//			const unsigned A2 = destPixel>>24;
			const unsigned R2 = (destPixel>>16)&0xff;
			const unsigned G2 = (destPixel>>8)&0xff;
			const unsigned B2 = destPixel&0xff; 

			const unsigned A1 = 0xff - (srcPixel>>24);
			const unsigned R1 = (srcPixel>>16)&0xff;
			const unsigned G1 = (srcPixel>>8)&0xff;
			const unsigned B1 = srcPixel&0xff; 

			unsigned R = ((R1*(0xff-A1))>>8) + ((R2*A1)>>8);
			unsigned G = ((G1*(0xff-A1))>>8) + ((G2*A1)>>8);
			unsigned B = ((B1*(0xff-A1))>>8) + ((B2*A1)>>8);

			if (R>255)R=255;
			if (G>255)G=255;
			if (B>255)B=255;

			const uint32_t result = (R<<16)|(G<<8)|B;
			pDest[iPixel] = result;
    }
}

void Sub32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels)
{
	const __m128i zero = _mm_setzero_si128();

	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
		const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pSrc[iPixel]), zero);
		const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iPixel]), zero);
		const __m128i delta = srcColor;
		const __m128i color = _mm_sub_epi16(destColor, delta);
		pDest[iPixel] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
	}
}

void Excl32(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels)
{
	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
			const uint32_t destPixel = pDest[iPixel];
			const uint32_t srcPixel  = pSrc[iPixel];

			const unsigned A2 = destPixel>>24;
			const unsigned R2 = (destPixel>>16)&0xff;
			const unsigned G2 = (destPixel>>8)&0xff;
			const unsigned B2 = destPixel&0xff; 

//			const unsigned A1 = srcPixel>>24;
			const unsigned R1 = (srcPixel>>16)&0xff;
			const unsigned G1 = (srcPixel>>8)&0xff;
			const unsigned B1 = srcPixel&0xff; 

//			const uint8_t R = R1 + R2 - 2*R1*R2/255;
//			const uint8_t G = G1 + G2 - 2*G1*G2/255;
//			const uint8_t B = B1 + B2 - 2*B1*B2/255;
			const unsigned R = R1 + R2 - ((2*R1*R2)>>8);
			const unsigned G = G1 + G2 - ((2*G1*G2)>>8);
			const unsigned B = B1 + B2 - ((2*B1*B2)>>8);
			const unsigned A = A2;

			const uint32_t result = (A<<24)|(R<<16)|(G<<8)|B;
			pDest[iPixel] = result;
    }
}

// no bit shifting here, I should do that more often instead of obeying to that built-in demoscene tic to shift wherever possible, that stopped making sense decades ago
// removing floating point calc. however is a sure shot, I also wonder if it might be faster to not have the branch and use a mask instead
VIZ_INLINE unsigned SoftLightBlend(uint8_t A, uint8_t B)
{
	const auto dA = (A/2)+64;
	if (B < 128)
	{
		return (2*dA*B)/256;
//		return 2 * ((A>>1) + 64)*(B/255.f);
	}
	else
	{
		return 255 - (2*(255-dA)*(255-B))/256;
//		return 255 - 2 * (255 - ((A>>1) + 64)) * (255 - B) / 255.f;
	}
}

void SoftLight32(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels)
{
	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
			const uint32_t destPixel = pDest[iPixel];
			const uint32_t srcPixel  = pSrc[iPixel];

			const unsigned A2 = destPixel>>24;
			const unsigned R2 = (destPixel>>16)&0xff;
			const unsigned G2 = (destPixel>>8)&0xff;
			const unsigned B2 = destPixel&0xff; 

//			const unsigned A1 = srcPixel>>24;
			const unsigned R1 = (srcPixel>>16)&0xff;
			const unsigned G1 = (srcPixel>>8)&0xff;
			const unsigned B1 = srcPixel&0xff; 

//			const uint8_t R = R1 + R2 - 2*R1*R2/255;
//			const uint8_t G = G1 + G2 - 2*G1*G2/255;
//			const uint8_t B = B1 + B2 - 2*B1*B2/255;
			const unsigned R = SoftLightBlend(R1, R2);
			const unsigned G = SoftLightBlend(G1, G2);
			const unsigned B = SoftLightBlend(B1, B2);
			const unsigned A = A2;

			const uint32_t result = (A<<24)|(R<<16)|(G<<8)|B;
			pDest[iPixel] = result;
    }
}

#if 0

// FIXME: optimize properly
void Overlay32(uint32_t *pDest, uint32_t *pSrc, unsigned numPixels) 
{
	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel) 
	{
		const uint32_t bottom = pDest[iPixel];
		const float bottomR = ((bottom>>16)&0xff)/255.f;
		const float bottomG = ((bottom>>8)&0xff)/255.f;
		const float bottomB = (bottom&0xff)/255.f;

		const uint32_t top = pSrc[iPixel];
		const float topR = ((top>>16)&0xff)/255.f;
		const float topG = ((top>>8)&0xff)/255.f;
		const float topB = (top&0xff)/255.f;

		float newR = bottomR < 0.5f ? (2.f * bottomR * topR) : (1.f - 2.f*(1.f-bottomR) * (1.f-topR));
		float newG = bottomG < 0.5f ? (2.f * bottomG * topG) : (1.f - 2.f*(1.f-bottomG) * (1.f-topG));
		float newB = bottomB < 0.5f ? (2.f * bottomB * topB) : (1.f - 2.f*(1.f-bottomB) * (1.f-topB));
		
		newR *= 255.f;
		newG *= 255.f;
		newB *= 255.f;

		pDest[iPixel] = unsigned(newR)<<16 | unsigned(newG)<<8 | unsigned(newB);
	}
}

// FIXME: the most ham-fisted, but always correct, version
void Overlay32A(uint32_t *pDest, uint32_t *pSrc, unsigned numPixels) 
{
	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel) 
	{
		const uint32_t bottom = pDest[iPixel];
		const float bottomR = ((bottom>>16)&0xff)/255.f;
		const float bottomG = ((bottom>>8)&0xff)/255.f;
		const float bottomB = (bottom&0xff)/255.f;

		const uint32_t top = pSrc[iPixel];
		const float topA = (top>>24)/255.f;
		const float topR = ((top>>16)&0xff)/255.f;
		const float topG = ((top>>8)&0xff)/255.f;
		const float topB = (top&0xff)/255.f;

		float newR = bottomR < 0.5f ? (2.f * bottomR * topR) : (1.f - 2.f*(1.f-bottomR) * (1.f-topR));
		float newG = bottomG < 0.5f ? (2.f * bottomG * topG) : (1.f - 2.f*(1.f-bottomG) * (1.f-topG));
		float newB = bottomB < 0.5f ? (2.f * bottomB * topB) : (1.f - 2.f*(1.f-bottomB) * (1.f-topB));

		newR = lerpf<float>(bottomR, newR, topA);
		newG = lerpf<float>(bottomG, newG, topA);
		newB = lerpf<float>(bottomB, newB, topA);

		newR *= 255.f;
		newG *= 255.f;
		newB *= 255.f;

		pDest[iPixel] = unsigned(newR)<<16 | unsigned(newG)<<8 | unsigned(newB);
	}
}

#else

// FIXME: optimize properly
void Overlay32(uint32_t *pDest, uint32_t *pSrc, unsigned numPixels)
{
	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
		const uint32_t bottom = pDest[iPixel];
		const float bottomR = float((bottom >> 16) & 0xff);
		const float bottomG = float((bottom >> 8) & 0xff);
		const float bottomB = float(bottom & 0xff);
		const uint32_t top = pSrc[iPixel];
		const float topR = float((top >> 16) & 0xff);
		const float topG = float((top >> 8) & 0xff);
		const float topB = float(top & 0xff);
		float newR = bottomR < 128.f ? (2.f * bottomR * topR / 255.f) : (255.f - 2.f * (255.f - bottomR) * (255.f - topR) / 255.f);
		float newG = bottomG < 128.f ? (2.f * bottomG * topG / 255.f) : (255.f - 2.f * (255.f - bottomG) * (255.f - topG) / 255.f);
		float newB = bottomB < 128.f ? (2.f * bottomB * topB / 255.f) : (255.f - 2.f * (255.f - bottomB) * (255.f - topB) / 255.f);
		pDest[iPixel] = ((unsigned)newR << 16) | ((unsigned)newG << 8) | (unsigned)newB;
	}
}

// FIXME: optimize properly
void Overlay32A(uint32_t *pDest, uint32_t *pSrc, unsigned numPixels)
{
	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
		const uint32_t bottom = pDest[iPixel];
		const float bottomR = float((bottom >> 16) & 0xff);
		const float bottomG = float((bottom >> 8) & 0xff);
		const float bottomB = float(bottom & 0xff);
		const uint32_t top = pSrc[iPixel];
		const float topA = float(top >> 24) / 255.f;
		const float topR = float((top >> 16) & 0xff);
		const float topG = float((top >> 8) & 0xff);
		const float topB = float(top & 0xff);
		float newR = bottomR < 128.f ? (2.f * bottomR * topR / 255.f) : (255.f - 2.f * (255.f - bottomR) * (255.f - topR) / 255.f);
		float newG = bottomG < 128.f ? (2.f * bottomG * topG / 255.f) : (255.f - 2.f * (255.f - bottomG) * (255.f - topG) / 255.f);
		float newB = bottomB < 128.f ? (2.f * bottomB * topB / 255.f) : (255.f - 2.f * (255.f - bottomB) * (255.f - topB) / 255.f);
		newR = lerpf<float>(bottomR, newR, topA);
		newG = lerpf<float>(bottomG, newG, topA);
		newB = lerpf<float>(bottomB, newB, topA);
		pDest[iPixel] = ((unsigned)newR << 16) | ((unsigned)newG << 8) | (unsigned)newB;
	}
}

#endif

// FIXME: optimize properly; especially this one is crazy suitable for SIMD!
void Darken32_50(uint32_t *pDest, uint32_t *pSrc, unsigned numPixels)
{
	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
			const uint32_t destPixel = pDest[iPixel];
			const uint32_t srcPixel  = pSrc[iPixel];

			const unsigned A2 = destPixel>>24;
			const unsigned R2 = (destPixel>>16)&0xff;
			const unsigned G2 = (destPixel>>8)&0xff;
			const unsigned B2 = destPixel&0xff; 

//			const unsigned A1 = srcPixel>>24;
			const unsigned R1 = (srcPixel>>16)&0xff;
			const unsigned G1 = (srcPixel>>8)&0xff;
			const unsigned B1 = srcPixel&0xff; 

			const unsigned DR = std::min<unsigned>(R1, R2);
			const unsigned DG = std::min<unsigned>(G1, G2);
			const unsigned DB = std::min<unsigned>(B1, B2);

			const unsigned R = (R2 + ((DR)))>>1;
			const unsigned G = (G2 + ((DG)))>>1;
			const unsigned B = (B2 + ((DB)))>>1;

			const unsigned A = A2;

			const uint32_t result = (A<<24)|(R<<16)|(G<<8)|B;
			pDest[iPixel] = result;
    }
}	

// FIXME: optimize properly
void TapeWarp32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float strength, float speed)
{
//	const float halfResX = xRes/2.f;
//	const float halfResY = yRes/2.f;
//	const float maxDist = sqrtf(halfResX*halfResX + halfResY*halfResY);

    #pragma omp parallel for schedule(static)
    for (int iY = 0; iY < int(yRes); ++iY)
    {
		const unsigned yIndex = iY*xRes;

        for (unsigned iX = 0; iX < xRes; ++iX)
        {
            const unsigned index = yIndex + iX;

            // FIXME: SIMD, fixed point!

//			const auto iXC = iX-halfResX;
//			const auto iYC = iY-halfResY;
//			const float distance = 1.f-sqrtf(iXC*iXC + iYC*iYC) / maxDist;
			constexpr float distance = 1.f;
			
            const float dX = lutsinf(iY*speed)*strength*distance;
            const float dY = lutcosf(iX*speed)*strength*distance;
            float tX = iX + dX;
            float tY = iY + dY;

			if (tX < 0.f)
				tX = 0.f;
			else if (tX >= xRes-1.f)
				tX = xRes - 2.f;
			if (tY < 0.f)
				tY = 0.f;
			else if (tY >= yRes-1.f)
				tY = yRes - 2.f;

			const auto U = ftofp24(tX);
			const auto V = ftofp24(tY);

			// prepare UVs
			const unsigned int U0 = U >> 8;
			const unsigned int V0 = (V >> 8) * kResX;
			const unsigned int fracU = (U & 0xff) * 0x01010101;
			const unsigned int fracV = (V & 0xff) * 0x01010101;

			// sample & return
			const auto sixteen = bsamp32_16(pSrc, U0, V0, U0+1, V0+kResX, fracU, fracV);
			pDest[index] = v2cISSE16(sixteen);
        }
    }
}

void MulSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels)
{
	const __m128i zero = _mm_setzero_si128();

	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
		const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pSrc[iPixel]), zero);
		const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iPixel]), zero);
		const __m128i delta = _mm_mullo_epi16(srcColor, destColor); 
		const __m128i color = _mm_srli_epi16(delta, 8);
		pDest[iPixel] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
	}
}

void MulSrc32A(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels)
{
	const __m128i zero = _mm_setzero_si128();

	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
		const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pSrc[iPixel]), zero);
		const __m128i alphaUnp = _mm_shufflelo_epi16(srcColor, 0xff);
		const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iPixel]), zero);
		const __m128i delta = _mm_mullo_epi16(alphaUnp, destColor); 
		const __m128i color = _mm_srli_epi16(delta, 8);
		pDest[iPixel] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
	}
}

void MixSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels)
{
	const __m128i zero = _mm_setzero_si128();

	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
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

// FIXME: optimize, that shuffle instruction sucks!
void BlitSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes)
{
	const __m128i zero = _mm_setzero_si128();

	#pragma omp parallel for schedule(static)
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

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < int(yRes); ++iY)
	{
		const uint32_t *srcPixel = pSrc + iY*srcResX;
		uint32_t *destPixel = pDest + iY*destResX;

		for (unsigned iX = 0; iX < srcResX; ++iX)
		{
			const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*srcPixel++), zero);
			const __m128i alphaUnp = _mm_srli_epi16(_mm_mullo_epi16(_mm_shufflelo_epi16(srcColor, 0xff), fixedAlphaUnp), 8);
			const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*destPixel), zero);
			const __m128i delta = _mm_mullo_epi16(alphaUnp, _mm_sub_epi16(srcColor, destColor));
			const __m128i color = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(destColor, 8), delta), 8);
			*destPixel++ = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
		}
	}
}

void BlitAdd32(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes)
{
	const __m128i zero = _mm_setzero_si128();

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < int(yRes); ++iY)
	{
		const uint32_t *srcPixel = pSrc + iY*srcResX;
		uint32_t *destPixel = pDest + iY*destResX;

		for (unsigned iX = 0; iX < srcResX; ++iX)
		{
			const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*srcPixel++), zero);
			const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*destPixel), zero);
			const __m128i delta = srcColor;
			const __m128i color = _mm_add_epi16(destColor, delta);
			*destPixel++ = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
		}
	}
}

void BlitAdd32A(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes, float alpha)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i fixedAlphaUnp = c2vISSE16(0x01010101 * unsigned(alpha*255.f));

	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < int(yRes); ++iY)
	{
		const uint32_t *srcPixel = pSrc + iY*srcResX;
		uint32_t *destPixel = pDest + iY*destResX;

		for (unsigned iX = 0; iX < srcResX; ++iX)
		{
			const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*srcPixel++), zero);
			const __m128i srcColorMod = _mm_srli_epi16(_mm_mullo_epi16(srcColor, fixedAlphaUnp), 8);
			const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*destPixel), zero);
			const __m128i delta = srcColorMod;
			const __m128i color = _mm_add_epi16(destColor, delta);
			*destPixel++ = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
		}
	}
}

void Fade32(uint32_t *pDest, unsigned int numPixels, uint32_t RGB, uint8_t alpha)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i alphaUnp = _mm_unpacklo_epi8(_mm_cvtsi32_si128(0x01010101 * alpha), zero);
	const __m128i srcColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(RGB), zero);

	#pragma omp parallel for schedule(static)
	for (int iPixel = 0; iPixel < int(numPixels); ++iPixel)
	{
		const __m128i destColor = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pDest[iPixel]), zero);
		const __m128i delta = _mm_mullo_epi16(alphaUnp, _mm_sub_epi16(srcColor, destColor));
		const __m128i color = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(destColor, 8), delta), 8);
		pDest[iPixel] = _mm_cvtsi128_si32(_mm_packus_epi16(color, zero));
	}
}
