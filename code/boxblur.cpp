
// cookiedough -- box blur filter (32-bit only, for now)

// this code was written in 2007 at Javeline, a former employer of mine
// has been cleaned up considerably, but it does contain a slight weight bias flaw (commented), and probably 1 or 2 more quirks

// 01/02/2023: 
// - vertical pass is a bit cache-unfriendly, optimize (ref. https://fgiesen.wordpress.com/2012/08/01/fast-blurs-2/)
// - get rid of the gratuitous redundancy (lots of duplicate code)
// - review SIMD

#include "main.h"
// #include "boxblur.h"

static uint32_t *s_pTempBuf = nullptr;

bool BoxBlur_Create()
{
	s_pTempBuf = static_cast<uint32_t *>(mallocAligned(kOutputSize, kAlignTo));

	return true;
}

void BoxBlur_Destroy() 
{
	freeAligned(s_pTempBuf);
	s_pTempBuf = nullptr;
}

// -- helper functions --

// convert 28:4 fixed point weight to 16-bit divisor
VIZ_INLINE uint32_t WeightToDiv(unsigned int weight)
{
	return ((65536*255)/weight)>>4;
}

// add pixel to accumulator
VIZ_INLINE void Add(__m128i &accumulator, __m128i &remainder, uint32_t pixel, unsigned int remainderShift)
{
	const __m128i pixelUnp = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pixel), _mm_setzero_si128());
	accumulator = _mm_adds_epu16(accumulator, remainder);
	remainder = _mm_srli_epi16(pixelUnp, remainderShift);
	accumulator = _mm_adds_epu16(accumulator, _mm_subs_epu16(pixelUnp, remainder));
}

// subtract pixel from accumulator
VIZ_INLINE void Sub(__m128i &accumulator, __m128i &remainder, uint32_t pixel, unsigned int remainderShift)
{
	const __m128i pixelUnp = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pixel), _mm_setzero_si128());
	accumulator = _mm_subs_epu16(accumulator, remainder);
	remainder = _mm_srli_epi16(pixelUnp, remainderShift);
	accumulator = _mm_subs_epu16(accumulator, _mm_subs_epu16(pixelUnp, remainder));
}

// divide accumulator by weight, pack it, and return pixel
VIZ_INLINE uint32_t Div(__m128i accumulator, __m128i weightDiv)
{
	return _mm_cvtsi128_si32(_mm_packus_epi16(_mm_mulhi_epu16(accumulator, weightDiv), _mm_setzero_si128()));
}

// -- horizontal implementation --

void HorizontalBoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength)
{
//	VIZ_ASSERT(pDest != pSrc);

	// calculate actual kernel span
	const float fKernelSpan = strength*256.f;
	unsigned kernelSpan = unsigned(fKernelSpan);
	kernelSpan = clampi(1, 255, kernelSpan);

	// derive edge details (even-sized kernels have subpixel edges)
	const bool subEdges = (kernelSpan & 1) == 0;
	const unsigned int edgeSpan = kernelSpan >> 1;
	const unsigned int remainderShift = 1 + (!subEdges*7);
	const unsigned int kernelMedian = edgeSpan + !subEdges;

	// calculate divisors for edge passes
	static __m128i edgeDivs[kResX];
	VIZ_ASSERT(kernelMedian < kResX);
	const unsigned int startWeight = (kernelMedian << 4) + (subEdges<<3); // FIXME: 0.5 weight bias during pre-pass
	for (unsigned int curWeight = startWeight, iDiv = 0; iDiv < kernelMedian; ++iDiv)
	{
		edgeDivs[iDiv] = _mm_set1_epi16(WeightToDiv(curWeight));
		curWeight += 16;
	}

	// full pass length & divisor
	const unsigned int fullPassLen = xRes - (kernelMedian+edgeSpan);
	const __m128i fullDiv = _mm_set1_epi16(WeightToDiv(kernelSpan << 4));

	// ready, set, blur!
	#pragma omp parallel for schedule(static)
	for (int iY = 0; iY < int(yRes); ++iY)
	{
		auto destIndex = iY*xRes;
		const uint32_t *pSrcLine = pSrc + destIndex;

		unsigned int addPos = 0;
		unsigned int subPos = 0;
		
		__m128i accumulator  = _mm_setzero_si128();
		__m128i addRemainder = _mm_setzero_si128();
		__m128i subRemainder = _mm_setzero_si128();
		
		// pre-read: bring accumulator op to edge weight
		for (unsigned int iX = 0; iX < edgeSpan; ++iX)
		{
			Add(accumulator, addRemainder, pSrcLine[addPos++], remainderShift);
		}

		// pre-pass: up to full weight
		for (unsigned int iX = 0; iX < kernelMedian; ++iX)
		{
			Add(accumulator, addRemainder, pSrcLine[addPos++], remainderShift);
			pDest[destIndex++] = Div(accumulator, edgeDivs[iX]);
		}
		
		// main pass
		for (unsigned int iX = 0; iX < fullPassLen; ++iX)
		{
			Add(accumulator, addRemainder, pSrcLine[addPos++], remainderShift);
			Sub(accumulator, subRemainder, pSrcLine[subPos++], remainderShift);
			pDest[destIndex++] = Div(accumulator, fullDiv);
		}

		// add additive remainder if needed (subtractive remainder is taken care of by Sub())
		if (subEdges)
			accumulator = _mm_adds_epu16(accumulator, addRemainder);
		
		// post-pass: back to median weight
		for (unsigned int iX = edgeSpan; iX > 0; --iX)
		{
			pDest[destIndex++] = Div(accumulator, edgeDivs[iX-1]);
			Sub(accumulator, subRemainder, pSrcLine[subPos++], remainderShift);
		}
	}
}

// -- (suboptimal) vertical implementation --

// straight copy of HorizontalBoxBlur32(), but simply reading & writing vertically:
// - you'd think this is absolutely terrible for cache and sure it violates the 64 and 128 bytes respectively
//   of the modern Intel and Apple M1/M2 cache line
// - but the level 1 cache on an average Ryzen is 64KB and at least 128KB on the Apple M, so, if the buffers aren't all that huge 
//   (which they are not), even in HD, 1920 pixels is 7,5KB per line; so for now, based on this, I won't optimize this
// - on a 486SX or so in the past you'd get into trouble as it had only 8KB which fit just enough for 256, but then you had more in L1 and
//   that generally ruined the party; by now memory is just so much faster and cache in abudance
// - and it's threaded as well

void VerticalBoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength)
{
//	VIZ_ASSERT(pDest != pSrc);

	// calculate actual kernel span
	const float fKernelSpan = strength*255.f;
	unsigned kernelSpan = unsigned(fKernelSpan);
	kernelSpan = clampi(1, 255, kernelSpan);

	// derive edge details (even-sized kernels have subpixel edges)
	const bool subEdges = (kernelSpan & 1) == 0;
	const unsigned int edgeSpan = kernelSpan >> 1;
	const unsigned int remainderShift = 1 + ((!subEdges)*7);
	const unsigned int kernelMedian = edgeSpan + !subEdges;

	// calculate divisors for edge passes
	static __m128i edgeDivs[kResY];
	VIZ_ASSERT(kernelMedian < kResY);
	const unsigned int startWeight = (kernelMedian << 4) + (subEdges << 3);
	for (unsigned int curWeight = startWeight, iDiv = 0; iDiv < kernelMedian; ++iDiv)
	{
		edgeDivs[iDiv] = _mm_set1_epi16(WeightToDiv(curWeight));
		curWeight += 16;
	}

	// full pass length & divisor
	const unsigned int fullPassLen = yRes - (kernelMedian+edgeSpan);
	const __m128i fullDiv = _mm_set1_epi16(WeightToDiv(kernelSpan << 4));

	// ready, set, blur!
	#pragma omp parallel for schedule(static)
	for (int iX = 0; iX < int(xRes); ++iX)
	{
		auto destIndex = iX;
		unsigned addPos = iX;
		unsigned subPos = iX;

		__m128i accumulator  = _mm_setzero_si128();
		__m128i addRemainder = _mm_setzero_si128();
		__m128i subRemainder = _mm_setzero_si128();
		
		// pre-read: bring accumulator op to edge weight
		for (unsigned int iY = 0; iY < edgeSpan; ++iY)
		{
			Add(accumulator, addRemainder, pSrc[addPos], remainderShift);
			addPos += xRes;
		}

		// pre-pass: up to full weight
		for (unsigned int iY = 0; iY < kernelMedian; ++iY)
		{
			pDest[destIndex] = Div(accumulator, edgeDivs[iY]);
			destIndex += xRes;

			Add(accumulator, addRemainder, pSrc[addPos], remainderShift);
			addPos += xRes;
		}
		
		// main pass
		for (unsigned int iY = 0; iY < fullPassLen; ++iY)
		{
			Add(accumulator, addRemainder, pSrc[addPos], remainderShift);
			addPos += xRes;

			Sub(accumulator, subRemainder, pSrc[subPos], remainderShift);
			subPos += xRes;

			pDest[destIndex] = Div(accumulator, fullDiv);
			destIndex += xRes;
		}
		
		// add additive remainder if needed (subtractive remainder is taken care of by Sub())
		if (subEdges)
			accumulator = _mm_adds_epu16(accumulator, addRemainder);

		// post-pass: back to median weight
		for (unsigned int iY = edgeSpan; iY > 0; --iY)
		{
			Sub(accumulator, subRemainder, pSrc[subPos], remainderShift);
			subPos += xRes;	

			pDest[destIndex] = Div(accumulator, edgeDivs[iY-1]);
			destIndex += xRes;
		}
	}
}

// -- what it says on the tin --

void BoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength)
{
	VIZ_ASSERT(xRes <= kResX && yRes <= kResY);

//	HorizontalBoxBlur32(s_pTempBuf, pSrc, xRes, yRes, strength);
//	VerticalBoxBlur32(pDest, s_pTempBuf, xRes, yRes, strength);

	// FIXME: temporary buffer (used above) added in "4 in the morning"-paranoia, we never read back
//	HorizontalBoxBlur32(pDest, pSrc, xRes, yRes, strength/kAspect);
	VerticalBoxBlur32(pDest, pSrc, xRes, yRes, strength);
}
