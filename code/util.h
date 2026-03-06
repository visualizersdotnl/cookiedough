
// cookiedough -- basic utilities: from assertions to color conversions 

#ifndef _UTIL_H_
#define _UTIL_H_

// assert macro (not so much an evangelism for this project, but it's available)
#ifdef _DEBUG
	#define VIZ_ASSERT(condition) if (!(condition)) DEBUG_TRAP
#else
	#define VIZ_ASSERT(condition)
#endif

#define VIZ_ASSERT_ALIGNED(buffer) VIZ_ASSERT(0 == (std::bit_cast<size_t>(buffer) & (kAlignTo-1)))
#define VIZ_ASSERT_NORM(value) VIZ_ASSERT(value >= 0.f && value <= 1.f)

// Windows+GCC inline macro (bruteforce for Windows+MSVC, normal otherwise)
// The "old" VIZ_INLINE functions depend on the tag (VIZ_INLINE) flagging them as static, this is not ideal but it's legacy
// Please use CKD_INLINE from here on out (02/03/2026)
#if defined(_WIN32) && defined(MSCV)
	#ifdef _DEBUG
		#define VIZ_INLINE static
		#define CKD_INLINE
	#else
		#define VIZ_INLINE static __forceinline
		#define CKD_INLINE __forceinline
	#endif
#elif defined(__GNUC__)
	#ifdef _DEBUG
		#define VIZ_INLINE static
		#define CKD_INLINE
	#else
		#define VIZ_INLINE static __inline 
		#define CKD_INLINE __inline
	#endif
#endif

// full C++ math library (last updated March 2026)
#include "Std3DMath-stripped/Math.h"

#include "bit-tricks.h"
#include "alloc-aligned.h"
#include "sincos-lut.h"
#include "q3-rsqrt.h"
#include "random.h"
#include "fast-cosine.h"
#include "synth-math-easings.h"

// I used to have a specific big-block fast memcpy() without write cache but I'm ditching that 
#define memcpy_fast memcpy

// only use for clearing (larger) buffers (does not cache writes)
VIZ_INLINE void memset32(void *pDest, int value, size_t numInts)
{
	// must be a multiple of 16 bytes -- use memset() for general purpose
	VIZ_ASSERT(!(numInts & 3));

	// an 8-byte boundary gaurantees correctly aligned writes
	VIZ_ASSERT(!(reinterpret_cast<size_t>(pDest) & 7));

	int *pInt = static_cast<int*>(pDest);
	while (numInts--) _mm_stream_si32(pInt++, value);
}

// function intended to slowly zoom in to backgrounds (an idea Nytrik had for Arrested Development)
void Zoom32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float scale);

// blend 32-bit color buffers
void Mix32(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels, uint8_t alpha);

// blend 32-bit color buffers as follows: A+B(1-ALPHA), discards dest. alpha
void MixOver32(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels);

// add 32-bit color buffers (source to/from destination)
void Add32(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels);

// subtract 32-bit color buffers (source to/from destination)
void Sub32(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels);

// Photoshop-style exclusion blend filter between two 32-bit color buffers (retains dest. alpha)
void Excl32(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels);

// Photoshop-style soft light blend filter between two 32-bit color buffers (retains dest. alpha)
void SoftLight32(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels);
void SoftLight32A(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels); // applies effect by src. alpha
void SoftLight32AA(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels, float alpha); // applies effect by alpha

// nonsensical warp effect applied to a 32-bit color buffer
void TapeWarp32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float strength, float speed);

// Photoshop-style overlay blend effect between two 32-bit color buffers (zeroes dest. alpha)
void Overlay32(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels);
void Overlay32A(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels); // applies effect by src. alpha

// Photoshop-style darken blend effect between two 32-bit color buffers (retains dest. alpha)
// result is blended 50% - this is specifically because I needed it this way (FIXME)
void Darken32_50(uint32_t *pDest, const uint32_t *pSrc, unsigned numPixels);

// multiply dest. buffer by either color or alpha of source buffer
void MulSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels);
void MulSrc32A(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels);

// blend 32-bit color buffers using the source buffer's alpha (the latter has a src. stride)
void MixSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned int numPixels);
void MixSrc32S(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned destResY, unsigned srcStride);

// blit 32-bit color buffer using the source buffer's alpha
// use this to composite graphics on top of effects for ex.
// BlitSrc32A(): alpha parameter will modulate source alpha ([0..1])
void BlitSrc32(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes);
void BlitSrc32A(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes, float alpha);

// same as above except it's simply additive
void BlitAdd32(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes);
void BlitAdd32A(uint32_t *pDest, const uint32_t *pSrc, unsigned destResX, unsigned srcResX, unsigned yRes, float alpha);

// fade 32-bit color buffer
void Fade32(uint32_t *pDest, unsigned int numPixels, uint32_t RGB, uint8_t alpha);

// convert 32-bit color to unpacked (16-bit) ISSE vector
CKD_INLINE static __m128i c2vISSE16(uint32_t color) { 
	return _mm_unpacklo_epi8(_mm_cvtsi32_si128(color), _mm_setzero_si128()); 
}

// convert 32-bit color to unpacked (32-bit) ISSE vector
CKD_INLINE static __m128i c2vISSE32(uint32_t color) { 
	return _mm_cvtepu8_epi32(_mm_cvtsi32_si128(color));
}

// unpack to floats (SSE 4.1)
CKD_INLINE static __m128 c2vfISSE(uint32_t color) 
{ 
	return  
		_mm_cvtepi32_ps(_mm_cvtepu8_epi32(_mm_cvtsi32_si128(color))); 
}

// 32-bit color <-> (I)SSE utility functions
// - fine for regular loads, preparatory work, edge cases and one-offs
// - please do not use the blends inside innerloops if you can help it (or at the very least label it with a FIXME)
// - the blends waste register bandwidth and perform an expensive ftol() for the floating point alpha

// convert unpacked (16-bit) ISSE vector to 32-bit color (2 pack instructions!)
CKD_INLINE static uint32_t v2cISSE16(__m128i color) { 
	return _mm_cvtsi128_si32(_mm_packus_epi16(color, _mm_setzero_si128())); 
}

// version of v2cISSE16() for 32-bit vectors (3 pack instructions!)
CKD_INLINE static uint32_t v2cISSE32(__m128i color) { 
	return _mm_cvtsi128_si32(_mm_packus_epi16(_mm_packus_epi32(color, _mm_setzero_si128()), _mm_setzero_si128())); 
}

// blend unpacked (16-bit) pixels
CKD_INLINE static __m128i vblendf(__m128i A, __m128i B, float alpha)
{
	VIZ_ASSERT(alpha >= 0.f && alpha <= 255.f);
	const uint32_t iAlpha =  uint32_t(alpha*255.f)*0x01010101;
	const __m128i alphaUnp = _mm_unpacklo_epi8(_mm_cvtsi32_si128(iAlpha), _mm_setzero_si128());
	const __m128i delta = _mm_mullo_epi16(alphaUnp, _mm_sub_epi16(B, A));
	const __m128i color = _mm_srli_epi16(_mm_add_epi16(_mm_slli_epi16(A, 8), delta), 8);
	return color;
}

// blend packed pixels, return unpacked (16-bit)
CKD_INLINE static __m128i c2vblendf(uint32_t A, uint32_t B, float alpha) {
	return vblendf(
			_mm_unpacklo_epi8(_mm_cvtsi32_si128(A), _mm_setzero_si128()),
			_mm_unpacklo_epi8(_mm_cvtsi32_si128(B), _mm_setzero_si128()),
			alpha
	);
}

// blend packed pixels
CKD_INLINE static uint32_t cblendf(uint32_t A, uint32_t B, float alpha) {
	return v2cISSE16(c2vblendf(A, B, alpha));
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

// simple floating point to 24:8 fixed point conversion (signed)
CKD_INLINE static int ftofp24(float value) { 
	return (int) (value*256.f); 
}

template <typename T>
CKD_INLINE static T ftofp(float value, unsigned leftShiftBit) {
	return T(value*float(1<<leftShiftBit));
}

// integer clamp (for curtailing Rocket parameters et cetera)
CKD_INLINE static int clampi(int min, int max, int value) {
	return std:: max<int>(min, std::min<int>(max, value));
}

#endif // _UTIL_H_
