
// cookiedough -- simple marched plasma

#include "main.h"
// #include "plasma.h"
#include "image.h"
// #include "bilinear.h"
#include "shadertoy-util.h"

bool Plasma_Create()
{
	return true;
}

void Plasma_Destroy()
{
}

void Plasma_Draw(uint32_t *pDest, float time, float delta)
{
	for (unsigned iY = 0; iY < kResY; ++iY)
	{
		for (unsigned iX = 0; iX < kResX; ++iX)
		{	
			Vector2 coordinate = Shadertoy_ToCoord(iX, iY);
			float aspectRatio = kAspect;
			*pDest++ = 0xff00ff;
		}
	}
}

