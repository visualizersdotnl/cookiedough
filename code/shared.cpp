
// cookiedough -- shared resources (FX)

#include "main.h"
// #include "shared.h"
#include "image.h"

uint32_t *g_pDesireLogo3 = nullptr;

__m128i g_gradientUnp[256];
uint32_t *g_renderTarget;

bool Shared_Create()
{
	// load logo #3
	g_pDesireLogo3 = Image_Load32("content/dsr_640x136.png");
	if (g_pDesireLogo3 == nullptr)
		return false;

	// create linear grayscale gradient (unpacked)
	for (int iPixel = 0; iPixel < 256; ++iPixel)
		g_gradientUnp[iPixel] = c2vISSE(iPixel * 0x01010101);

	// allocate 640x480 render target
	g_renderTarget = new uint32_t[640*480];

	return true;
}

void Shared_Destroy()
{
	delete[] g_pDesireLogo3;
	delete[] g_renderTarget;
}
