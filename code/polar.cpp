
// cookiedough -- polar blits (640x480 -> kResX*kResY)

#include "main.h"
// #include "polar.h"
#include "bilinear.h"

static int *s_pPolarMap = nullptr;
static int *s_pInvPolarMap = nullptr;

bool Polar_Create()
{
	s_pPolarMap    = static_cast<int*>(mallocAligned(kResX*kResY*sizeof(int)*2, kCacheLine));
	s_pInvPolarMap = static_cast<int*>(mallocAligned(kResX*kResY*sizeof(int)*2, kCacheLine));

	// calculate cartesian-to-polar transform map (RT resolution -> kResX*kResY)
	const float maxDist = sqrtf(kHalfResX*kHalfResX + kHalfResY*kHalfResY);
	for (unsigned int iPixel = 0, iY = 0; iY < kResY; ++iY)
	{
		for (unsigned int iX = 0; iX < kResX; ++iX)
		{
			const float X = (float) iX - kHalfResX;
			const float Y = (float) iY - kHalfResY;
			const float distance = sqrtf(X*X + Y*Y) / maxDist;
			float theta = atan2f(Y, X);
			theta += kPI;
			theta /= kPI*2.f;
			const float U    = distance * (kTargetResX-1.f);
			const float invU = (1.f-distance) * (kTargetResX-1.f);
			const float V    = theta * (kTargetResY-1.f);

			// non-zero edges must be patched in absence of tiling logic
			// it's a simple reverse (read previous pixel first & invert weight)

			if (U == kTargetResX-1.f)
				s_pInvPolarMap[iPixel] = s_pPolarMap[iPixel] = (kTargetResX-2)<<8 | 0xff;
			else
			{
				s_pPolarMap[iPixel] = ftof24(U);
				s_pInvPolarMap[iPixel] = ftof24(invU);
			}

			if (V == kTargetResY-1.f)
				s_pInvPolarMap[iPixel+1] = s_pPolarMap[iPixel+1] = (kTargetResY-2)<<8 | 0xff;
			else
				s_pInvPolarMap[iPixel+1] = s_pPolarMap[iPixel+1] = ftof24(V);

			iPixel += 2;
		}
	}

	return true;
}

void Polar_Destroy() 
{
	freeAligned(s_pPolarMap);
	freeAligned(s_pInvPolarMap);
}

void Polar_Blit(const uint32_t *pSrc, uint32_t *pDest, bool inverse /* = false */)
{
	int *pRead = (!inverse) ? s_pPolarMap : s_pInvPolarMap;
	for (unsigned int iPixel = 0; iPixel < kResX*kResY; ++iPixel)
	{
		const int U = *pRead++;
		const int V = *pRead++;

		// prepare UVs
		const unsigned int U0 = U >> 8;
		const unsigned int V0 = (V >> 8) * kTargetResX; // remove multiply (FIXME)
		const unsigned int fracU = (U & 0xff) * 0x01010101;
		const unsigned int fracV = (V & 0xff) * 0x01010101;

		// sample & store
		const __m128i color = bsamp32(pSrc, U0, V0, U0+1, V0+kTargetResX, fracU, fracV);
		pDest[iPixel] = v2cISSE(color);
//		pDest[iPixel] = pSrc[U0+V0];
	}
}
