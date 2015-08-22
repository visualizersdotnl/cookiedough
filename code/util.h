
// cookiedough -- basic utilities

#ifndef _UTIL_H_
#define _UTIL_H_

// assert macro (not so much an evangelism for this project, but it's available)
#ifdef _DEBUG
	#define VIZ_ASSERT(condition) if (!(condition)) __debugbreak()
#else
	#define VIZ_ASSERT(condition)
#endif

// PI
const float kPI = 3.1415926535897932384626433832795f;

// unsigned integer is-power-of-2 check
inline const bool IsPow2(unsigned value)
{
	return value != 0 && !(value & (value - 1));
}

#if _WIN64

// memcpy_fast() and memset32() are both MMX & inline assembler (unsupported)
// also, according to a blog post, memcpy() has been optimized (http://blogs.msdn.com/b/vcblog/archive/2009/11/02/visual-c-code-generation-in-visual-studio-2010.aspx)

#define memcpy_fast memcpy

__forceinline void memset32(void *pDest, uint32_t value, size_t numInts)
{
	uint32_t *pDest32 = static_cast<uint32_t*>(pDest);
	while (numInts--) *pDest32++ = value;
}

#else

// memcpy_fast() & memset32() are MMX-optimized versions of memcpy() and memset()
// - functions are not 64-bit compliant (size_t won't fit in a 32-bit register) 
// - *only* intended for copying large batches (size restrictions apply)
// - preferrably align addresses to 8 byte boundary

void memcpy_fast(void *pDest, const void *pSrc, size_t numBytes); // only for multiples of 64 bytes
void memset32(void *pDest, uint32_t value, size_t numInts);       // only for multiples of 16 bytes (or 4 integers)

#endif // _WIN64

// blend 32-bit color buffers
void Mix32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels, uint8_t alpha);

// blend 32-bit color buffers using the source buffer's alpha
void MixSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels);

// fade 32-bit color buffer
void Fade32(uint32_t *pDest, unsigned int numPixels, uint32_t RGB, uint8_t alpha);

// convert 32-bit color to unpacked (16-bit) ISSE vector
__forceinline __m128i c2vISSE(uint32_t color) 
{
	return  _mm_unpacklo_epi8(_mm_cvtsi32_si128(color), _mm_setzero_si128());
}

// convert unpacked (16-bit) ISSE vector to 32-bit color
__forceinline uint32_t v2cISSE(__m128i color)
{
	return _mm_cvtsi128_si32(_mm_packus_epi16(color, _mm_setzero_si128()));
}

// ISSE vector (16-bit) minimum
__forceinline __m128i vminISSE(__m128i A, __m128i B)
{
	// SSE4
	return _mm_min_epu16(A, B);

	// SSE2
//	const __m128i mask = _mm_cmplt_epi16(A, B);
//	return _mm_add_epi16(_mm_and_si128(mask, A), _mm_andnot_si128(mask, B));
}

// simple floating point to 24:8 fixed point conversion
__forceinline int ftof24(float value) { return (int) (value*256.f); }

#endif // _UTIL_H_
