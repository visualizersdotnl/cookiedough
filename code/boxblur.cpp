
// cookiedough -- optimized 32-bit multi-pass (gaussian approximation) blur

// 06/03/2026:
// - [x] horizontal SIMD-optimized blur pass works perfectly
// - [x] move horizontal blur pass into a function that blurs a L1-cache sized chunk into the scratch buffer(s) for all passes required
// - [x] use OpenMP split loop up into parallel chunks
// - [/] implement horizontal/vertical/full pass
// - [ ] implement 2007 blur using this one

#include "main.h"
// #include "boxblur.h"

constexpr size_t kMaxRes = 2048;
constexpr size_t kScratchSize = kMaxRes*kMaxRes;

static uint32_t *s_pScratch = nullptr;

bool BoxBlur_Create()
{
	s_pScratch = (uint32_t *) mallocAligned(kScratchSize*sizeof(uint32_t), kAlignTo);
	return true;	
}

void BoxBlur_Destroy() 
{
	freeAligned(s_pScratch);
}

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
	const __m128i low64  = _mm_unpacklo_epi32(iSum, _mm_setzero_si128()); // | A | 0 | R | 0 |
	const __m128i high64 = _mm_unpackhi_epi32(iSum, _mm_setzero_si128()); // | G | 0 | B | 0 |
	
	const __m128i broadcast = iScale; // _mm_set1_epi64x(iScale);

	// divide by kernel size, then the horizontal-add sums it back to a single register: | A | R | G | B |
	return _mm_hadd_epi32(
		_mm_srli_epi64(_mm_mul_epu32(low64, broadcast), 22),
		_mm_srli_epi64(_mm_mul_epu32(high64, broadcast), 22));
}

// alignas(kAlignTo) static thread_local int64_t s_iSpanScales[kMaxRes/2];
alignas(kAlignTo) static thread_local __m128i s_iSpanScales[kMaxRes/2];

// horizontal blur pass (used to implement each variant)
static const uint32_t *HorzBlur32(uint32_t *pDest, uint32_t *pScratch, const uint32_t *pSrc, unsigned xRes, unsigned numLines, float radius, unsigned numPasses)
{
	VIZ_ASSERT_ALIGNED(pSrc);
	VIZ_ASSERT(xRes <= kMaxRes && numLines <= kMaxRes);
//	VIZ_ASSERT(xRes*numLines < kScratchSize);
	VIZ_ASSERT_NORM(radius);

	// transform radius to pixels (reserve room for 1 subpixel edge on either side)
//	radius *= (xRes-1)/2.f;
	
	// WIP
	radius = 8.f;
	numPasses = 3;

	const unsigned iSpan = unsigned(radius);
	VIZ_ASSERT(iSpan < kMaxRes/2);

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

	const uint32_t *pRead  = pSrc;
	uint32_t *pWrite       = pDest;

	for (unsigned iPass = 0; iPass < numPasses; ++iPass)
	{
		// Y
		for (unsigned iY = 0; iY < numLines; ++iY)
		{
			// X
			auto writeIdx = iY*xRes;
			const uint32_t *pLine = pRead + writeIdx;

			__m128i iSum = _mm_setzero_si128();

			unsigned tail = 0, head = 0;

			// calculate sum at first pixel (median)
			for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
			{
//				const uint32_t pixel = pLine[head];
//				sumA += GetA(pixel);
//				sumR += GetR(pixel);
//				sumG += GetG(pixel);
//				sumB += GetB(pixel);
//				iSumA += pixel >> 24;
//				iSumR += (pixel >> 16) & 0xff;
//				iSumG += (pixel >> 8) & 0xff;
//				iSumB += pixel & 0xff;
				iSum = _mm_add_epi32(iSum, c2vISSE32(pLine[head]));
				++head;
			}

//			const uint32_t subPixel = cblend(0, pLine[head], alpha);
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
				_mm_srai_epi32(_mm_mul_epu32(c2vISSE32(pLine[head]), iAlpha), 8));

//			float curScale = scale*0.5f;
//			const float dScale = (scale*0.5f)/iSpan;

			__m128i 
				headA, headB,
				tailA, tailB;

			// work up to full kernel size
			headA = c2vISSE32(pLine[head+1]);
			for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
			{
//				pWrite[writeIdx] = ftoc(sumA*curScale, sumR*curScale, sumG*curScale, sumB*curScale);
//				pWrite[writeIdx] = itoc(iSumA, iSumR, iSumG, iSumB, s_iSpanScales[iPixel]);
//				curScale += dScale;

				pWrite[writeIdx] = v2cISSE32(iDiv(iSum, s_iSpanScales[iPixel]));
				++writeIdx;

//				const uint32_t addHead = cblend(pSrcLine[head+1], pLine[head+2], alpha);
//				sumA += GetA(addHead);
//				sumR += GetR(addHead);
//				sumG += GetG(addHead);
//				sumB += GetB(addHead);
//				iSumA += addHead >> 24;
//				iSumR += (addHead >> 16) & 0xff;
//				iSumG += (addHead >> 8) & 0xff;
//				iSumB += addHead & 0xff;
				
				headB = c2vISSE32(pLine[head+2]);
				iSum = iAdd(iSum, iAlpha, headA, headB);
				headA = headB;

				++head;
			}

			// normalize
//			curScale = scale;

			// full sliding window
			tailA = c2vISSE32(pLine[tail]);
			for (unsigned iPixel = 0; iPixel < xRes - iSpan*2; ++iPixel)
			{
//				pWrite[writeIdx] = ftoc(sumA*curScale, sumR*curScale, sumG*curScale, sumB*curScale);
//				pWrite[writeIdx] = itoc(iSumA, iSumR, iSumG, iSumB, iScale);

				pWrite[writeIdx] = v2cISSE32(iDiv(iSum, iScale));
				++writeIdx;

//				const uint32_t addHead = cblend(pLine[head+1], pLine[head+2], alpha);
//				sumA += GetA(addHead);
//				sumR += GetR(addHead);
//				sumG += GetG(addHead);
//				sumB += GetB(addHead);
//				iSumA += addHead >> 24;
//				iSumR += (addHead >> 16) & 0xff;
//				iSumG += (addHead >> 8) & 0xff;
//				iSumB += addHead & 0xff;

				headB = c2vISSE32(pLine[head+2]);
				iSum = iAdd(iSum, iAlpha, headA, headB);
				headA = headB;
				++head;

//				const uint32_t subTail = cblend(pLine[tail], pLine[tail+1], alpha);
//				sumA -= GetA(subTail);
//				sumR -= GetR(subTail);
//				sumG -= GetG(subTail);
//				sumB -= GetB(subTail);
//				iSumA -= subTail >> 24;
//				iSumR -= (subTail >> 16) & 0xff;
//				iSumG -= (subTail >> 8) & 0xff;
//				iSumB -= subTail & 0xff;
				
				tailB = c2vISSE32(pLine[tail+1]);
				iSum = iSub(iSum, iAlpha, tailA, tailB);
				tailA = tailB;
				++tail;
			}

			// work back down to median
			for (unsigned iPixel = iSpan; iPixel > 0; --iPixel)
			{
//				pWrite[writeIdx] = ftoc(sumA*curScale, sumR*curScale, sumG*curScale, sumB*curScale);
//				pWrite[writeIdx] = itoc(iSumA, iSumR, iSumG, iSumB, s_iSpanScales[iPixel-1]);
//				curScale -= dScale;

				pWrite[writeIdx] = v2cISSE32(iDiv(iSum, s_iSpanScales[iPixel-1]));
				++writeIdx;

//				const uint32_t subTail = cblend(pLine[tail+1], pLine[tail], alpha);
//				sumA -= GetA(subTail);
//				sumR -= GetR(subTail);
//				sumG -= GetG(subTail);
//				sumB -= GetB(subTail);
//				iSumA -= subTail >> 24;
//				iSumR -= (subTail >> 16) & 0xff;
//				iSumG -= (subTail >> 8) & 0xff;
//				iSumB -= subTail & 0xff;

				tailB = c2vISSE32(pLine[tail+1]);
				iSum = iSub(iSum, iAlpha, tailA, tailB);
				tailA = tailB;
				++tail;
			}
		}

		pRead = pWrite;
		pWrite = (0 == (iPass & 1))
			? pScratch
			: pDest;
	}

	return pRead;
}

void BoxBlur_Horz32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses)
{
	VIZ_ASSERT(xRes > 0 && yRes > 0 && numPasses > 0);

	// as optimal as it gets without injecting parallelism straight into HorzBlur32() which would defeat the design
	#pragma omp parallel
	{
		const int iThread    = omp_get_thread_num();
		const int numThreads = omp_get_num_threads();

		const unsigned rowsPerThread = (yRes + numThreads-1)/numThreads;

		const unsigned yStart = iThread*rowsPerThread;
		const unsigned yEnd = std::min(yStart+rowsPerThread, yRes);

		const unsigned numLines = yEnd-yStart;
		const unsigned offset = yStart*xRes;

		const uint32_t *pResult = HorzBlur32(
			pDest+offset,
			s_pScratch+offset,
			pSrc+offset,
			xRes,
			yEnd-yStart,
			radius,
			numPasses);

		if (0 == (numPasses & 1))
			// FIXME: use streaming writes here (MOVNTQ)
			memcpy(pDest+offset, s_pScratch+offset, sizeof(uint32_t)*numLines*xRes);
	}
}

void BoxBlur_Vert32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses)
{
	// idea: transpose -> blur -> transpose (uncached writes)
}

void BoxBlur_32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses)
{
	// idea: two sequential blur->transpose passes
}
