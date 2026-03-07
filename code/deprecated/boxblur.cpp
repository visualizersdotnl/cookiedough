
// cookiedough -- deprecated 2007 box blur implementation 

// - written in 2007, not tended to since, yet still used in 'Arrested Development' (2023)
// - it has unnecessary limitations: kernel size, odd and overflow-prone fixed point arithmetic
// - it has a cache-unfriendly naive vertical pass (though it's potentially not that bad for relatively small images)

#include "../main.h"
// #include "boxblur.h"

VIZ_INLINE uint32_t WeightToDiv(unsigned int weight)
{
	return ((65536*256)/weight)>>4;
}

// add pixel to accumulator
VIZ_INLINE void Add(__m128i &accumulator, __m128i &remainder, uint32_t pixel, unsigned int remainderShift)
{
	__m128i pixelUnp = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pixel), _mm_setzero_si128());
//	const __m128i alphaUnp = _mm_shufflelo_epi16(pixelUnp, 0xff);
//	pixelUnp = _mm_srli_epi16(_mm_mullo_epi16(pixelUnp, alphaUnp), 8);
	accumulator = _mm_adds_epu16(accumulator, remainder);
	remainder = _mm_srli_epi16(pixelUnp, remainderShift);
	accumulator = _mm_adds_epu16(accumulator, _mm_subs_epu16(pixelUnp, remainder));
}

// subtract pixel from accumulator
VIZ_INLINE void Sub(__m128i &accumulator, __m128i &remainder, uint32_t pixel, unsigned int remainderShift)
{
	__m128i pixelUnp = _mm_unpacklo_epi8(_mm_cvtsi32_si128(pixel), _mm_setzero_si128());
//	const __m128i alphaUnp = _mm_shufflelo_epi16(pixelUnp, 0xff);
//	pixelUnp = _mm_srli_epi16(_mm_mullo_epi16(pixelUnp, alphaUnp), 8);
	accumulator = _mm_subs_epu16(accumulator, remainder);
	remainder = _mm_srli_epi16(pixelUnp, remainderShift);
	accumulator = _mm_subs_epu16(accumulator, _mm_subs_epu16(pixelUnp, remainder));
}

// divide accumulator by weight, pack it, and return pixel
VIZ_INLINE uint32_t Div(__m128i accumulator, __m128i weightDiv)
{
	return _mm_cvtsi128_si32(_mm_packus_epi16(_mm_mulhi_epu16(accumulator, weightDiv), _mm_setzero_si128()));
}

void HorizontalBoxBlur32(
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
	const unsigned int remainderShift = 1 + (!subEdges*7);
	const unsigned int kernelMedian = edgeSpan + !subEdges;

	// calculate divisors for edge passes
	static __m128i edgeDivs[256];
	VIZ_ASSERT(kernelMedian < 256);
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
			Sub(accumulator, subRemainder, pSrcLine[subPos++], remainderShift);
			pDest[destIndex++] = Div(accumulator, edgeDivs[iX-1]);
		}
	}
}

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
	static __m128i edgeDivs[256];
	VIZ_ASSERT(kernelMedian < 256);
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
			Add(accumulator, addRemainder, pSrc[addPos], remainderShift);
			addPos += xRes;

			pDest[destIndex] = Div(accumulator, edgeDivs[iY]);
			destIndex += xRes;
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

void BoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength)
{
	HorizontalBoxBlur32(pDest, pSrc, xRes, yRes, strength);
	VerticalBoxBlur32(pDest, pDest, xRes, yRes, strength);
}
