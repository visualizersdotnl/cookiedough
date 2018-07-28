
// cookiedough -- old school 4x4 map interpolation blitters (mainly for computationally heavy effects)

#include "main.h"
#include "map-blitter.h"
#include "cspan.h"

bool MapBlitter_Create()
{
	return true;
}

void MapBlitter_Destroy()
{

}

// FIXME: tight ISSE loop!
void MapBlitter_Colors(uint32_t* pDest, uint32_t* pSrc)
{
	for (int iY = 0; iY < kFXMapResY; ++iY)
	{
		__m128i from = c2vISSE(pSrc[0]);
		for (int iX = 1; iX < kFXMapResX; ++iX)
		{
			__m128i to = c2vISSE(pSrc[iX]);
			cspanISSE_noclip(pDest, 1, 4, from, to);
			from = to;

			pDest += 4;
		}

		pSrc += kFXMapResX;
		pDest += 4 + 3*kResX;
	}
}

/*
void MapBlitter_UV(...)
{
}
*/
