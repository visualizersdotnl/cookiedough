
// cookiedough -- shared resources (FX)

#include "main.h"
#include "image.h"

uint32_t *g_pDesireLogo3 = nullptr;

__m128i g_gradientUnp[256];
uint32_t *g_renderTarget = nullptr;

bool Shared_Create()
{
	// load logo #3
	g_pDesireLogo3 = Image_Load32("assets/dsr_640x136.png");
	if (g_pDesireLogo3 == nullptr)
		return false;

	// create linear grayscale gradient (unpacked)
	for (int iPixel = 0; iPixel < 256; ++iPixel)
		g_gradientUnp[iPixel] = c2vISSE(iPixel * 0x01010101);

	// allocate render target
	g_renderTarget = static_cast<uint32_t*>(mallocAligned(kTargetBytes, kCacheLine));

	return true;
}

void Shared_Destroy()
{
	Image_Free(g_pDesireLogo3);
	freeAligned(g_renderTarget);
}
