
// cookiedough -- shared resources (FX)

#include "main.h"
#include "image.h"

__m128i g_gradientUnp[256];
uint32_t *g_renderTarget = nullptr;
uint32_t *g_NytrikMexico = nullptr;

bool Shared_Create()
{
	// create linear grayscale gradient (unpacked)
	for (int iPixel = 0; iPixel < 256; ++iPixel)
		g_gradientUnp[iPixel] = c2vISSE16(iPixel * 0x01010101);

	// allocate render target
	g_renderTarget = static_cast<uint32_t*>(mallocAligned(kTargetBytes, kCacheLine));

	// load Nytrik's "Mexico" logo
	g_NytrikMexico = Image_Load32("assets/TPB-Mexico-logo-01.jpg");
	if (g_NytrikMexico == NULL)
		return false;

	return true;
}

void Shared_Destroy()
{
	freeAligned(g_renderTarget);
}
