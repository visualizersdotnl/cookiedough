
// cookiedough -- polar blit (640x480 -> kResX*kResY)

#include "main.h"
// #include "polar.h"
#include "bilinear.h"

static int *s_pPolarMap = nullptr;

bool Polar_Create()
{
	s_pPolarMap = static_cast<int*>(mallocAligned(kResX*kResY*sizeof(int)*2, kCacheLine));

	// calculate cartesian-to-polar transform map (640x480 -> kResX*kResY)
	const float maxDist = sqrtf(kHalfResX*kHalfResX + kHalfResY*kHalfResY);
	for (unsigned int iPixel = 0, iY = 0; iY < kResY; ++iY)
	{
		for (unsigned int iX = 0; iX < kResX; ++iX)
		{
			const float X = (float) iX - kHalfResX;
			const float Y = (float) iY - kHalfResY;
			/* const */ float distance = sqrtf(X*X + Y*Y) / maxDist;
//			distance = 1.f-distance;
			float theta = atan2f(Y, X);
			theta += kPI;
			theta /= kPI*2.f;
			const float U = distance * 639.f;
			const float V =    theta * 479.f;

			// non-zero edges must be patched in absence of tiling logic
			// it's a simple reverse (read previous pixel first & invert weight)

			if (U == 639.f)
				s_pPolarMap[iPixel] = 638<<8 | 0xff;
			else
				s_pPolarMap[iPixel] = ftof24(U);

			if (V == 479.f)
				s_pPolarMap[iPixel+1] = 478<<8 | 0xff;
			else
				s_pPolarMap[iPixel+1] = ftof24(V);

			iPixel += 2;
		}
	}

	return true;
}

void Polar_Destroy() 
{
	freeAligned(s_pPolarMap);
}

void Polar_Blit(const uint32_t *pSrc, uint32_t *pDest)
{
	int *pRead = s_pPolarMap;
	for (unsigned int iPixel = 0; iPixel < kResX*kResY; ++iPixel)
	{
		const int U = *pRead++;
		const int V = *pRead++;

		// prepare UVs
		const unsigned int U0 = U >> 8;
		const unsigned int V0 = (V >> 8) * 640; // remove multiply (FIXME)
		const unsigned int fracU = (U & 0xff) * 0x01010101;
		const unsigned int fracV = (V & 0xff) * 0x01010101;

		// sample & store
		const __m128i color = bsamp32(pSrc, U0, V0, U0+1, V0+640, fracU, fracV);
		pDest[iPixel] = v2cISSE(color);
//		pDest[iPixel] = pSrc[U0+V0];
	}
}
