
// cookiedough -- optimized 32-bit multi-pass (gaussian approximation) blur

// 06/03/2026:
// - [x] horizontal SIMD-optimized blur pass works perfectly
// - move horizontal blur pass into a function that blurs a L1-cache sized chunk into the scratch buffer(s) for all passes required
// - use read/write/copy logistics (see below) to implement horizontal, vertical and full blur optimally using the one horizontal pass
// - dispatch OpenMP to parallelize each L1-cache sized chunk
// - implement 2007 blur using this one

// about the 2007 blur (don't use it, it's just here for 'Arrested Development'):
// - I wrote this blur in my Javeline days in 2007 and I haven't seriously tended to it since
// - it has unnecessary limitations: kernel size, odd and overflow-prone fixed point arithmetic
// - it has a cache-unfriendly naive vertical pass (though it's potentially not that bad for relatively small images)

#include "main.h"
// #include "boxblur.h"

static uint32_t *s_pScratch[2] =  { nullptr };

bool BoxBlur_Create()
{
	// for multiple blur passes I'll flip-flop between these, which should keep the L1 cache occupied
	s_pScratch[0] = (uint32_t *) mallocAligned(kCacheL1, kAlignTo);
	s_pScratch[1] = (uint32_t *) mallocAligned(kCacheL1, kAlignTo);

	return true;	
}

void BoxBlur_Destroy() 
{
	freeAligned(s_pScratch[0]);
	freeAligned(s_pScratch[1]);
}

// -- 2026 blur --

// ref.
CKD_INLINE static float GetA(uint32_t color) { return (color >> 24); }
CKD_INLINE static float GetR(uint32_t color) { return (color >> 16) & 0xff; }
CKD_INLINE static float GetG(uint32_t color) { return (color >>  8) & 0xff; }
CKD_INLINE static float GetB(uint32_t color) { return (color >>  0) & 0xff; }

// ref.
CKD_INLINE static uint32_t ftoc(float A, float R, float G, float B) {
	A = clampf(0.f, 255.f, A);
	R = clampf(0.f, 255.f, R);
	G = clampf(0.f, 255.f, G);
	B = clampf(0.f, 255.f, B);
	return uint8_t(A) << 24 | uint8_t(R) << 16 | uint8_t(G) << 8 | uint8_t(B); 
}

// ref.
CKD_INLINE static uint32_t itoc(unsigned A, unsigned R, unsigned G, unsigned B, int64_t iScale) {
	A = unsigned(iScale*int64_t(A)>>22);
	R = unsigned(iScale*int64_t(R)>>22);
	G = unsigned(iScale*int64_t(G)>>22);
	B = unsigned(iScale*int64_t(B)>>22);
	return uint8_t(A) << 24 | uint8_t(R) << 16 | uint8_t(G) << 8 | uint8_t(B); 
}

CKD_INLINE static __m128i iAdd(__m128i iSum, __m128i iAlpha, __m128i A, __m128i B)
{
	// sum += lerp(head[1], head[2], alpha)
	return _mm_add_epi32(
		iSum,
		_mm_add_epi32(
			A,
			_mm_srai_epi32(_mm_mul_epi32(_mm_sub_epi32(B, A), iAlpha), 8)));
}

CKD_INLINE static __m128i iSub(__m128i iSum, __m128i iAlpha, __m128i A, __m128i B)
{
	// sum -= lerp(tail[0], tail[1], alpha)
	return _mm_sub_epi32(
		iSum,
		_mm_add_epi32(
			A,
			_mm_srai_epi32(_mm_mul_epi32(_mm_sub_epi32(B, A), iAlpha), 8)));
}

CKD_INLINE static __m128i iDiv(__m128i iSum, __m128i iScale)
{
	const __m128i low64  = _mm_unpacklo_epi32(iSum, _mm_setzero_si128()); // | A | R | 
	const __m128i high64 = _mm_unpackhi_epi32(iSum, _mm_setzero_si128()); // | G | B |
	
	const __m128i broadcast = iScale; // _mm_set1_epi64x(iScale);

	// divide by kernel size, then the horizontal-add sums it back to a single register: | A | R | G | B |
	return _mm_hadd_epi32(
		_mm_srli_epi64(_mm_mul_epu32(low64, broadcast), 22),
		_mm_srli_epi64(_mm_mul_epu32(high64, broadcast), 22));
}

// FIXME: size up
// alignas(kAlignTo) static int64_t s_iSpanScales[kResX/2];
alignas(kAlignTo) static __m128i s_iSpanScales[kResX/2];

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
	VIZ_ASSERT(iSpan < kResX/2);

	// calculate sum divider and alpha
	const float scale = 1.f/(2.f*radius + 1.f);
	const float alpha = radius-iSpan;
//	const int64_t iScale = ftofp<int64_t>(scale, 22); // 10:22
	const __m128i iScale = _mm_set1_epi64x(ftofp<int64_t>(scale, 22)); // 10:22
	const __m128i iAlpha = _mm_cvtsi32_si128(ftofp24(alpha)*0x01010101);

	// calculate span scales
	const float halfScale = scale*0.5f;
	const float dScale = halfScale/iSpan;
	for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
		s_iSpanScales[iPixel] = _mm_set1_epi64x(ftofp<int64_t>(halfScale + iPixel*dScale, 22));

	// Y
	for (unsigned iY = 0; iY < yRes; ++iY)
	{
		// idea: if you have multiple passes and use both L1-sized scratch buffers (double buffering style) you'll roll cleanly with the cache
//		for (unsigned iPass = 0; iPass < numPasses; ++iPass)
		{
			// X
			auto destIndex = iY*xRes;
			const uint32_t *pSrcLine = pSrc + destIndex;

//			float 
//				sumA = 0.f,
//				sumR = 0.f, 
//				sumG = 0.f, 
//				sumB = 0.f;

//			unsigned
//				iSumA = 0,
//				iSumR = 0,
//				iSumG = 0,
//				iSumB = 0;

			__m128i iSum = _mm_setzero_si128();

			unsigned tail = 0, head = 0;

			// calculate sum at first pixel (median)
			for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
			{
//				const uint32_t pixel = pSrcLine[head];

//				sumA += GetA(pixel);
//				sumR += GetR(pixel);
//				sumG += GetG(pixel);
//				sumB += GetB(pixel);

//				iSumA += pixel >> 24;
//				iSumR += (pixel >> 16) & 0xff;
//				iSumG += (pixel >> 8) & 0xff;
//				iSumB += pixel & 0xff;

				iSum = _mm_add_epi32(iSum, c2vISSE32(pSrcLine[head]));

				++head;
			}

//			const uint32_t subPixel = cblend(0, pSrcLine[head], alpha);

//			sumA += GetA(subPixel);
//			sumR += GetR(subPixel);
//			sumG += GetG(subPixel);
//			sumB += GetB(subPixel);

//			iSumA += subPixel >> 24;
//			iSumR += (subPixel >> 16) & 0xff;
//			iSumG += (subPixel >> 8) & 0xff;
//			iSumB += subPixel & 0xff;

			iSum = _mm_add_epi32(
				iSum,
				_mm_srai_epi32(_mm_mul_epu32(c2vISSE32(pSrcLine[head]), iAlpha), 8));

//			float curScale = scale*0.5f;
//			const float dScale = (scale*0.5f)/iSpan;

			__m128i 
				headA, headB,
				tailA, tailB;

			// work up to full kernel
			headA = c2vISSE32(pSrcLine[head+1]);
			for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
			{
//				pDest[destIndex] = ftoc(sumA*curScale, sumR*curScale, sumG*curScale, sumB*curScale);
//				pDest[destIndex] = itoc(iSumA, iSumR, iSumG, iSumB, s_iSpanScales[iPixel]);
//				curScale += dScale;

				pDest[destIndex] = v2cISSE32(iDiv(iSum, s_iSpanScales[iPixel]));
				++destIndex;

//				const uint32_t addHead = cblend(pSrcLine[head+1], pSrcLine[head+2], alpha);

//				sumA += GetA(addHead);
//				sumR += GetR(addHead);
//				sumG += GetG(addHead);
//				sumB += GetB(addHead);

//				iSumA += addHead >> 24;
//				iSumR += (addHead >> 16) & 0xff;
//				iSumG += (addHead >> 8) & 0xff;
//				iSumB += addHead & 0xff;
				
				headB = c2vISSE32(pSrcLine[head+2]);
				iSum = iAdd(iSum, iAlpha, headA, headB);
				headA = headB;

				++head;
			}

			// normalize
//			curScale = scale;

			tailA = c2vISSE32(pSrcLine[tail]);

			// full sliding window
			for (unsigned int iPixel = 0; iPixel < xRes - iSpan*2; ++iPixel)
			{
//				pDest[destIndex] = ftoc(sumA*curScale, sumR*curScale, sumG*curScale, sumB*curScale);
//				pDest[destIndex] = itoc(iSumA, iSumR, iSumG, iSumB, iScale);

				pDest[destIndex] = v2cISSE32(iDiv(iSum, iScale));
				++destIndex;

//				const uint32_t addHead = cblend(pSrcLine[head+1], pSrcLine[head+2], alpha);

//				sumA += GetA(addHead);
//				sumR += GetR(addHead);
//				sumG += GetG(addHead);
//				sumB += GetB(addHead);

//				iSumA += addHead >> 24;
//				iSumR += (addHead >> 16) & 0xff;
//				iSumG += (addHead >> 8) & 0xff;
//				iSumB += addHead & 0xff;

				headB = c2vISSE32(pSrcLine[head+2]);
				iSum = iAdd(iSum, iAlpha, headA, headB);
				headA = headB;

				++head;

//				const uint32_t subTail = cblend(pSrcLine[tail], pSrcLine[tail+1], alpha);

//				sumA -= GetA(subTail);
//				sumR -= GetR(subTail);
//				sumG -= GetG(subTail);
//				sumB -= GetB(subTail);

//				iSumA -= subTail >> 24;
//				iSumR -= (subTail >> 16) & 0xff;
//				iSumG -= (subTail >> 8) & 0xff;
//				iSumB -= subTail & 0xff;
				
				tailB = c2vISSE32(pSrcLine[tail+1]);
				iSum = iSub(iSum, iAlpha, tailA, tailB);
				tailA = tailB;

				++tail;
			}

			// work back down to median
			for (unsigned int iPixel = iSpan; iPixel > 0; --iPixel)
			{
//				pDest[destIndex] = ftoc(sumA*curScale, sumR*curScale, sumG*curScale, sumB*curScale);
//				pDest[destIndex] = itoc(iSumA, iSumR, iSumG, iSumB, s_iSpanScales[iPixel-1]);
//				curScale -= dScale;

				pDest[destIndex] = v2cISSE32(iDiv(iSum, s_iSpanScales[iPixel-1]));
				++destIndex;

//				const uint32_t subTail = cblend(pSrcLine[tail+1], pSrcLine[tail], alpha);

//				sumA -= GetA(subTail);
//				sumR -= GetR(subTail);
//				sumG -= GetG(subTail);
//				sumB -= GetB(subTail);

//				iSumA -= subTail >> 24;
//				iSumR -= (subTail >> 16) & 0xff;
//				iSumG -= (subTail >> 8) & 0xff;
//				iSumB -= subTail & 0xff;

				tailB = c2vISSE32(pSrcLine[tail+1]);
				iSum = iSub(iSum, iAlpha, tailA, tailB);
				tailA = tailB;

				++tail;
			}
		}
	}
}

void BoxBlur_Vert32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses)
{
	// idea (lift as much common logic out of horizontal):
	// - transpose N columns to scratch buffer
	// - perform horizontal blur on scratch buffer
	// - transpose from scratch buffer to dest. image (uncached writes)
	// - repeat
	// - factor in multiple passes
}

void BoxBlur_32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses)
{
	// idea (lift as much common logic out of horizontal):
	// - fill scratch buffer to fit (N lines)
	// - perform horizontal blur on scratch buffer
	// - transpose from scratch buffer to dest. image (uncached writes)
	// - repeat / factor in multiple passes
	// - do this entire process twice, and you'll have effectively blurred, transposed, blurred and transposed back
}

// -- 2007 blur --

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
