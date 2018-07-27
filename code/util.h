
// cookiedough -- basic utilities

#ifndef _UTIL_H_
#define _UTIL_H_

// size of cache line
constexpr size_t kCacheLine = sizeof(size_t)<<3;

// assert macro (not so much an evangelism for this project, but it's available)
#ifdef _DEBUG
	#define VIZ_ASSERT(condition) if (!(condition)) __debugbreak()
#else
	#define VIZ_ASSERT(condition)
#endif

// Windows+GCC inline macro (bruteforce in Windows, normal otherwise)
#ifdef  _WIN32
	#ifdef _DEBUG
		#define VIZ_INLINE static
	#else
		#define VIZ_INLINE __forceinline
	#endif
#else // elif defined(__GNUC__)
	#ifdef _DEBUG
		#define VIZ_INLINE static
	#else
		#define VIZ_INLINE inline
	#endif
#endif

#include "bit-tricks.h"
#include "alloc-aligned.h"

// full 3D math library (last updated 26/07/2018)
#include "../3rdparty/Std3DMath-stripped/Math.h"

// memcpy_fast() & memset32() are optimized versions of memcpy() and memset()
// - *only* intended for copying large batches (restrictions apply), explicitly *bypassing the write cache*
// - align addresses to 8 byte boundary

#if _WIN64

	// memcpy() has been optimized: http://blogs.msdn.com/b/vcblog/archive/2009/11/02/visual-c-code-generation-in-visual-studio-2010.aspx
	#define memcpy_fast memcpy

#else

// only for multiples of 64 bytes
void memcpy_fast(void *pDest, const void *pSrc, size_t numBytes); 

#endif

VIZ_INLINE void memset32(void *pDest, int value, size_t numInts)
{
	// must be a multiple of 16 bytes -- use memset() for general purpose
	VIZ_ASSERT(!(numInts & 3));

	// an 8-byte boundary gaurantees correctly aligned writes
	VIZ_ASSERT(!(reinterpret_cast<size_t>(pDest) & 7));

	int *pInt = static_cast<int*>(pDest);
	while (numInts--) _mm_stream_si32(pInt++, value);
}

// blend 32-bit color buffers
void Mix32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels, uint8_t alpha);

// add 32-bit color buffers (source onto destination)
void Add32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels);

// blend 32-bit color buffers using the source buffer's alpha
void MixSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels);

// fade 32-bit color buffer
void Fade32(uint32_t *pDest, unsigned int numPixels, uint32_t RGB, uint8_t alpha);

// convert 32-bit color to unpacked (16-bit) ISSE vector
VIZ_INLINE __m128i c2vISSE(uint32_t color) { return  _mm_unpacklo_epi8(_mm_cvtsi32_si128(color), _mm_setzero_si128()); }

// convert unpacked (16-bit) ISSE vector to 32-bit color
VIZ_INLINE uint32_t v2cISSE(__m128i color) { return _mm_cvtsi128_si32(_mm_packus_epi16(color, _mm_setzero_si128())); }

// mix 2 unpacked 32-bit pixels by alpha
VIZ_INLINE __m128i MixPixels32(__m128i A, __m128i B, float alpha)
{
	const uint32_t iAlpha =  uint32_t(alpha*255.f)*0x01010101;
	const __m128i zero = _mm_setzero_si128();
	const __m128i alphaUnp = _mm_unpacklo_epi8(_mm_cvtsi32_si128(iAlpha), zero);
	const __m128i delta = _mm_mullo_epi16(alphaUnp, _mm_sub_epi16(B, A));
	const __m128i color = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(A, 8), delta), 8);
	return color;
}

// mix 2 32-bit pixels by alpha
VIZ_INLINE uint32_t MixPixels32(uint32_t A, uint32_t B, float alpha)
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i vB = _mm_unpacklo_epi8(_mm_cvtsi32_si128(B), zero);
	const __m128i vA = _mm_unpacklo_epi8(_mm_cvtsi32_si128(A), zero);
	return v2cISSE(MixPixels32(vA, vB, alpha));
}

// ISSE vector (16-bit) minimum
VIZ_INLINE __m128i vminISSE(__m128i A, __m128i B)
{
	// SSE4
	return _mm_min_epu16(A, B);

	// SSE2
//	const __m128i mask = _mm_cmplt_epi16(A, B);
//	return _mm_add_epi16(_mm_and_si128(mask, A), _mm_andnot_si128(mask, B));
}

// simple floating point to 24:8 fixed point conversion
VIZ_INLINE int ftof24(float value) { return (int) (value*256.f); }

#endif // _UTIL_H_
