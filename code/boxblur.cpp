
// cookiedough -- optimized 32-bit box blur (suitable for gaussian approximation)

// 04/03/2026: implementing new blur, to do/don't forget:
// - [x] implement and test floating point horizontal blur prototype
// - [x] use fixed point arithmetic
// - [x] implement passes to approx. gaussian look
// - [ ] use 10:22 SIMD fixed point calculations
// - [ ] retain reference impl.
// - [ ] honour number of passes
// - [ ] implement cache-optimized version that uses two horizontal blur + transpose passes to do a full blur
// - [ ] plug OpenMP back in (try to make sure OpenMP respects the approx. L1 cache granularity)

// about the 2007 blur (don't use it, it's just here for 'Arrested Development'):
// - I wrote this blur in my Javeline days in 2007 and I haven't seriously tended to it since
// - it has unnecessary limitations: fixed kernel size, odd and overflow-prone fixed point arithmetic
// - it has a cache-unfriendly naive vertical pass

#include "main.h"
#include "util.h"
// #include "boxblur.h"

bool BoxBlur_Create()
{
	return true;	
}

void BoxBlur_Destroy() 
{
}

CKD_INLINE static float GetA(uint32_t color) { return (color >> 24); }
CKD_INLINE static float GetR(uint32_t color) { return (color >> 16) & 0xff; }
CKD_INLINE static float GetG(uint32_t color) { return (color >>  8) & 0xff; }
CKD_INLINE static float GetB(uint32_t color) { return (color >>  0) & 0xff; }

CKD_INLINE static uint32_t ftoc(float A, float R, float G, float B) {
	A = clampf(0.f, 255.f, A);
	R = clampf(0.f, 255.f, R);
	G = clampf(0.f, 255.f, G);
	B = clampf(0.f, 255.f, B);
	return uint8_t(A) << 24 | uint8_t(R) << 16 | uint8_t(G) << 8 | uint8_t(B); 
}

CKD_INLINE static uint32_t itoc(unsigned A, unsigned R, unsigned G, unsigned B, unsigned iScale) {
	A = unsigned(iScale*uint64_t(A)>>22);
	R = unsigned(iScale*uint64_t(R)>>22);
	G = unsigned(iScale*uint64_t(G)>>22);
	B = unsigned(iScale*uint64_t(B)>>22);
	return uint8_t(A) << 24 | uint8_t(R) << 16 | uint8_t(G) << 8 | uint8_t(B); 
}

void BoxBlur_Horz32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses)
{
	// buffers must be a multiple of 4x4 pixels (realistic and prevents us drowning in useless edge cases)
	VIZ_ASSERT(xRes > 3 && (xRes & 3) == 0);
	VIZ_ASSERT(yRes > 3 && (yRes & 3) == 0);

	// transform radius to pixels (reserve room for 1 subpixel edge on either side)
	VIZ_ASSERT_NORM(radius);
//	radius *= (xRes-1)/2.f;

	// WIP
	radius = 8.f;

	VIZ_ASSERT(numPasses > 0);
	
	// WIP
	numPasses = 2;

	const unsigned iSpan = unsigned(radius);

	// calculate sum divider and alpha
	const float scale = 1.f/(2.f*radius + 1.f);
	const float alpha = radius-iSpan;
//	const int iAlpha = ftofp24(alpha);
	const unsigned iScale = ftofp<unsigned>(scale, 22); // 10:22

	for (unsigned iY = 0; iY < yRes; ++iY)
	{
		for (unsigned iPass = 0; iPass < numPasses; ++iPass)
		{
			auto destIndex = iY*xRes;
			const uint32_t *pSrcLine = pSrc + destIndex;

			float 
				sumA = 0.f,
				sumR = 0.f, 
				sumG = 0.f, 
				sumB = 0.f;

			unsigned
				iSumA = 0,
				iSumR = 0,
				iSumG = 0,
				iSumB = 0;

			unsigned tail = 0, head = 0;

			// calculate sum at first pixel (median)
			for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
			{
				const uint32_t pixel = pSrcLine[head];
				sumA += GetA(pixel);
				sumR += GetR(pixel);
				sumG += GetG(pixel);
				sumB += GetB(pixel);
				iSumA += pixel >> 24;
				iSumR += (pixel >> 16) & 0xff;
				iSumG += (pixel >> 8) & 0xff;
				iSumB += pixel & 0xff;
				++head;
			}

			const uint32_t subPixel = MixPixels32(0, pSrcLine[head], alpha);
			sumA += GetA(subPixel);
			sumR += GetR(subPixel);
			sumG += GetG(subPixel);
			sumB += GetB(subPixel);
			iSumA += subPixel >> 24;
			iSumR += (subPixel >> 16) & 0xff;
			iSumG += (subPixel >> 8) & 0xff;
			iSumB += subPixel & 0xff;

			float curScale = scale*0.5f;
			const float dScale = (scale*0.5f)/iSpan;

			// work up to full kernel
			for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
			{
				pDest[destIndex] = ftoc(sumA*curScale, sumR*curScale, sumG*curScale, sumB*curScale);
				curScale += dScale;
				++destIndex;

				const uint32_t addHead = MixPixels32(pSrcLine[head+1], pSrcLine[head+2], alpha);
				sumA += GetA(addHead);
				sumR += GetR(addHead);
				sumG += GetG(addHead);
				sumB += GetB(addHead);
				iSumA += addHead >> 24;
				iSumR += (addHead >> 16) & 0xff;
				iSumG += (addHead >> 8) & 0xff;
				iSumB += addHead & 0xff;
				++head;
			}

			// normalize
			curScale = scale;

			// full sliding window
			for (unsigned int iPixel = 0; iPixel < xRes - iSpan*2; ++iPixel)
			{
//				pDest[destIndex] = ftoc(sumA*curScale, sumR*curScale, sumG*curScale, sumB*curScale);
				pDest[destIndex] = itoc(iSumA, iSumR, iSumG, iSumB, iScale);
				++destIndex;

				const uint32_t addHead = MixPixels32(pSrcLine[head+1], pSrcLine[head+2], alpha);
				sumA += GetA(addHead);
				sumR += GetR(addHead);
				sumG += GetG(addHead);
				sumB += GetB(addHead);
				iSumA += addHead >> 24;
				iSumR += (addHead >> 16) & 0xff;
				iSumG += (addHead >> 8) & 0xff;
				iSumB += addHead & 0xff;
				++head;

				const uint32_t subTail = MixPixels32(pSrcLine[tail], pSrcLine[tail+1], alpha);
				sumA -= GetA(subTail);
				sumR -= GetR(subTail);
				sumG -= GetG(subTail);
				sumB -= GetB(subTail);
				iSumA -= subTail >> 24;
				iSumR -= (subTail >> 16) & 0xff;
				iSumG -= (subTail >> 8) & 0xff;
				iSumB -= subTail & 0xff;
				++tail;
			}

			// work back down to median
			for (unsigned int iPixel = 0; iPixel < iSpan; ++iPixel)
			{
				pDest[destIndex] = ftoc(sumA*curScale, sumR*curScale, sumG*curScale, sumB*curScale);
				curScale -= dScale;
				++destIndex;

				const uint32_t subTail = MixPixels32(pSrcLine[tail+1], pSrcLine[tail], alpha);
				sumA -= GetA(subTail);
				sumR -= GetR(subTail);
				sumG -= GetG(subTail);
				sumB -= GetB(subTail);
				iSumA -= subTail >> 24;
				iSumR -= (subTail >> 16) & 0xff;
				iSumG -= (subTail >> 8) & 0xff;
				iSumB -= subTail & 0xff;
				++tail;
			}
		}
	}
}

// -- 2007 blur: helper functions --

// convert 28:4 fixed point weight to 16-bit divisor
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

// -- 2007 blur: horizontal implementation --

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

// -- 2007 blur: vertical implementation (suboptimal copy of horizontal version which is plain cache-unfriendly) --

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

// -- what it says on the tin --

void BoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength)
{
	VIZ_ASSERT(xRes <= kResX && yRes <= kResY);

	HorizontalBoxBlur32(pDest, pSrc, xRes, yRes, strength);
	VerticalBoxBlur32(pDest, pDest, xRes, yRes, strength);
}
