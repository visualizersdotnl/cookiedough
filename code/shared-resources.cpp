
// cookiedough -- shared resources (FX)

#include "main.h"
#include "image.h"

__m128i g_gradientUnp16[kNumGradients];

uint32_t *g_renderTarget[kNumRenderTargets] = { nullptr };

uint32_t *g_pNytrikMexico = nullptr;
uint32_t *g_pXboxLogoTPB = nullptr;

bool Shared_Create()
{
	// create linear grayscale gradient (unpacked)
	for (int iPixel = 0; iPixel < kNumGradients; ++iPixel)
	{
		g_gradientUnp16[iPixel] = c2vISSE16(iPixel * 0x01010101);
	}

	// allocate render targets
	for (unsigned iTarget = 0; iTarget < kNumRenderTargets; ++iTarget)
		g_renderTarget[iTarget] = static_cast<uint32_t*>(mallocAligned(kTargetBytes, kAlignTo));

	// load Nytrik's "Mexico" logo
	g_pNytrikMexico = Image_Load32("assets/TPB-Mexico-logo-01.jpg");
	if (g_pNytrikMexico == NULL)
		return false;

	// load Alien's TPB-02 Xbox logo
	g_pXboxLogoTPB = Image_Load32("assets/tpb_xbox_tp-263x243.png");
	if (g_pXboxLogoTPB == NULL)
		return false;

	return true;
}

void Shared_Destroy()
{
	for (unsigned iTarget = 0; iTarget < kNumRenderTargets; ++iTarget)
		freeAligned(g_renderTarget[iTarget]);
}
