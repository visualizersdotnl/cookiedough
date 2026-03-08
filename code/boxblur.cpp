
// cookiedough -- optimized 32-bit multi-pass (gaussian approximation) blur

// 06/03/2026:
// - [x] horizontal SIMD-optimized blur pass works perfectly
// - [x] move horizontal blur pass into a function that blurs a L1-cache sized chunk into the scratch buffer(s) for all passes required
// - [x] use OpenMP split loop up into parallel chunks
// - [/] implement horizontal/vertical/full pass

#include "main.h"
// #include "boxblur.h"

constexpr size_t kMaxRes = 2048;
constexpr size_t kScratchSize = kMaxRes*kMaxRes;

static uint32_t *s_pScratch[2] = { nullptr };

bool BoxBlur_Create()
{
	s_pScratch[0] = (uint32_t *) mallocAligned(kScratchSize*sizeof(uint32_t), kAlignTo);
	s_pScratch[1] = (uint32_t *) mallocAligned(kScratchSize*sizeof(uint32_t), kAlignTo);
	return true;	
}

void BoxBlur_Destroy() 
{
	for (auto *pAlloc : s_pScratch)
		freeAligned(pAlloc);
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
	const __m128i AR64 = _mm_unpacklo_epi32(iSum, _mm_setzero_si128()); // | A | 0 | R | 0 |
	const __m128i GB64 = _mm_unpackhi_epi32(iSum, _mm_setzero_si128()); // | G | 0 | B | 0 |

	// divide by kernel size, then the horizontal-add sums it back to a single register: | A | R | G | B |
	return _mm_hadd_epi32(
		_mm_srli_epi64(_mm_mul_epu32(AR64, iScale), 22),
		_mm_srli_epi64(_mm_mul_epu32(GB64, iScale), 22));
}

// alignas(kAlignTo) static thread_local int64_t s_iSpanScales[kMaxRes/2];
alignas(kAlignTo) static thread_local __m128i s_iSpanScales[kMaxRes/2];

// workhorse: horizontal blur pass(es) with optional write transpose
static void HorzBlur32(
	uint32_t *pDest, uint32_t *pScratch, const uint32_t *pSrc, 
	unsigned xRes, unsigned yRes, 
	unsigned writeStrideCol, unsigned writeStrideRow,
	float radius, unsigned numPasses)
{
	VIZ_ASSERT_ALIGNED(pSrc);
	VIZ_ASSERT(xRes <= kMaxRes && yRes <= kMaxRes);
//	VIZ_ASSERT(xRes*numLines < kScratchSize);
	VIZ_ASSERT_NORM(radius);

	// transform radius to pixels (reserve room for 1 subpixel edge on either side)
//	radius *= (xRes-1)/2.f;
	
	// WIP
	radius = 8.f;
	numPasses = 2;

	const unsigned iSpan = unsigned(radius);
	VIZ_ASSERT(iSpan < kMaxRes/2);

	// calculate sum divider and alpha
	const float scale = 1.f/(2.f*radius + 1.f);
	const float alpha = radius-iSpan;
	const __m128i iScale = _mm_set1_epi64x(ftofp<int64_t>(scale, 22)); // 10:22
	const __m128i iAlpha = _mm_cvtsi32_si128(ftofp24(alpha)*0x01010101);

	// calculate span scales
	const float halfScale = scale*0.5f;
	const float dScale = halfScale/iSpan;
	for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
		s_iSpanScales[iPixel] = _mm_set1_epi64x(ftofp<int64_t>(halfScale + iPixel*dScale, 22));

	const bool evenNumPasses = 0 == (numPasses & 1);

	const uint32_t *pRead = pSrc;

	if (true == evenNumPasses)
		// even (2): scratch -> dest. -> scratch -> dest vs. uneven (3): dest. -> scratch -> dest.
		std::swap(pDest, pScratch);

	for (unsigned iPass = 0; iPass < numPasses; ++iPass)
	{
		unsigned colStride = xRes, rowStride = 1;

		if (iPass == numPasses-1)
		{
			// last pass: write as instructed
			colStride = writeStrideCol;
			rowStride = writeStrideRow;
		}

		// Y
		for (unsigned iY = 0; iY < yRes; ++iY)
		{
			// X (calculated here for potential OpenMP-parallelization)
			unsigned writeIdx = iY*colStride;
			const uint32_t *pLine = pRead + iY*xRes;

			__m128i iSum = _mm_setzero_si128();

			unsigned tail = 0, head = 0;

			// calculate sum at first pixel (median)
			for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
				iSum = _mm_add_epi32(iSum, c2vISSE32(pLine[head++]));

			iSum = _mm_add_epi32(
				iSum,
				_mm_srai_epi32(_mm_mul_epu32(c2vISSE32(pLine[head]), iAlpha), 8));

			__m128i 
				headA, headB,
				tailA, tailB;

			// work up to full kernel size
			headA = c2vISSE32(pLine[head+1]);
			for (unsigned iPixel = 0; iPixel < iSpan; ++iPixel)
			{
				pDest[writeIdx] = v2cISSE32(iDiv(iSum, s_iSpanScales[iPixel]));
				writeIdx += rowStride;
				
				headB = c2vISSE32(pLine[head+2]);
				iSum = iAdd(iSum, iAlpha, headA, headB);
				headA = headB;

				++head;
			}

			// full sliding window
			tailA = c2vISSE32(pLine[tail]);
			for (unsigned iPixel = 0; iPixel < xRes - iSpan*2; ++iPixel)
			{
				pDest[writeIdx] = v2cISSE32(iDiv(iSum, iScale));
				writeIdx += rowStride;

				headB = c2vISSE32(pLine[head+2]);
				iSum = iAdd(iSum, iAlpha, headA, headB);
				headA = headB;
				++head;
				
				tailB = c2vISSE32(pLine[tail+1]);
				iSum = iSub(iSum, iAlpha, tailA, tailB);
				tailA = tailB;
				++tail;
			}

			// work back down to median
			for (unsigned iPixel = iSpan; iPixel > 0; --iPixel)
			{
				pDest[writeIdx] = v2cISSE32(iDiv(iSum, s_iSpanScales[iPixel-1]));
				writeIdx += rowStride;

				tailB = c2vISSE32(pLine[tail+1]);
				iSum = iSub(iSum, iAlpha, tailA, tailB);
				tailA = tailB;
				++tail;
			}
		}

		pRead = pDest;
		std::swap(pDest, pScratch);
	}
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

		HorzBlur32(
			pDest+offset, s_pScratch[0]+offset, pSrc+offset,
			xRes, yEnd-yStart,
			xRes, 1,			
			radius, numPasses);
	}
}

void BoxBlur_Vert32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses)
{
	// idea: transpose -> blur -> transpose (uncached writes)
}

void BoxBlur_32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses)
{
	VIZ_ASSERT(xRes > 0 && yRes > 0 && numPasses > 0);

	// horizontal blur -> transpose image
	HorzBlur32(
		s_pScratch[1], s_pScratch[0], pSrc, 
		xRes, yRes,
		1, yRes,
		radius, numPasses);
	
	// vertrical blur -> transpose back
	HorzBlur32(
		pDest, s_pScratch[0], s_pScratch[1], 
		yRes, xRes,
		1, xRes,
		radius, numPasses);
}
