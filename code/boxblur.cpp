
// cookiedough -- box blur filter (32-bit only, for now)

// this code was written in 2007 at Javeline, a former employer of mine
// has been cleaned up considerably, but it does contain a slight weight bias flaw (commented)

#include "main.h"
// #include "boxblur.h"

// convert 28:4 fixed point weight to 16-bit divisor
VIZ_INLINE uint32_t WeightToDiv(unsigned int weight)
{
	return (65535*256 / weight) >> 4;
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

void HorizontalBoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength)
{
	VIZ_ASSERT(xRes <= 640); // FIXME: support variable resolutions

	// calculate actual kernel span
	const unsigned int kernelSpan = (unsigned int) (strength * (float) xRes);
	if (kernelSpan == 0) 
	{
		return;
	}
	else 
	{
		VIZ_ASSERT(kernelSpan <= xRes);
	}

	// derive edge details (even-sized kernels have subpixel edges)
	const bool subEdges = (kernelSpan & 1) == 0;
	const unsigned int edgeSpan = kernelSpan >> 1;
	const unsigned int remainderShift = 1 + (!subEdges*7);
	const unsigned int kernelMedian = edgeSpan + !subEdges;

	// calculate divisors for edge passes
	static __m128i edgeDivs[640];
	VIZ_ASSERT(kernelMedian < 640);
	const unsigned int startWeight = (kernelMedian << 4) + (subEdges << 3); // see note below!
	for (unsigned int curWeight = startWeight, iDiv = 0; iDiv < kernelMedian; ++iDiv)
	{
		edgeDivs[iDiv] = _mm_set1_epi16(WeightToDiv(curWeight));
		curWeight += 16;
	}

	// note:
	// the algorithm has a 0.5 weight bias during the pre-pass: this must be fixed!
	// until then the edge divisors account for it, it shouldn't really have visible consequence

	// full pass length & divisor
	const unsigned int fullPassLen = xRes - (kernelMedian+edgeSpan);
	const __m128i fullDiv = _mm_set1_epi16(WeightToDiv(kernelSpan << 4));

	// ready, set, blur!
	const uint32_t *pSrcLine = pSrc;
	for (unsigned int iY = 0; iY < yRes; ++iY)
	{
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
			*pDest++ = Div(accumulator, edgeDivs[iX]);
		}
		
		// main pass
		for (unsigned int iX = 0; iX < fullPassLen; ++iX)
		{
			Add(accumulator, addRemainder, pSrcLine[addPos++], remainderShift);
			Sub(accumulator, subRemainder, pSrcLine[subPos++], remainderShift);
			*pDest++ = Div(accumulator, fullDiv);
		}
		
		// add additive remainder if needed (subtractive remainder is taken care of by Sub())
		if (subEdges)
			accumulator = _mm_adds_epu16(accumulator, addRemainder);
		
		// post-pass: back to median weight
		for (unsigned int iX = edgeSpan; iX > 0; --iX)
		{
			Sub(accumulator, subRemainder, pSrcLine[subPos++], remainderShift);
			*pDest++ = Div(accumulator, edgeDivs[iX-1]);
		}

		pSrcLine += xRes;
	}
}
