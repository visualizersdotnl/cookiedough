
// cookiedough -- shared resources (FX)

#include "main.h"
#include "image.h"

__m128i g_gradientUnp16[kNumGradients];
__m128i g_gradientUnp32[kNumGradients];

uint32_t *g_renderTarget[kNumRenderTargets] = { nullptr };

uint32_t *g_pNytrikMexico = nullptr;
uint32_t *g_pXboxLogoTPB = nullptr;
uint32_t *g_pToyPusherTiles[8] = { nullptr };

uint32_t *g_pCredits[3] = { nullptr };

bool Shared_Create()
{
	// create linear grayscale gradient (unpacked)
	for (int iPixel = 0; iPixel < kNumGradients; ++iPixel)
	{
		g_gradientUnp16[iPixel] = c2vISSE16(iPixel * 0x01010101);
//		g_gradientUnp32[iPixel] = ...
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

	// load ToyPusher tiles (128x128)
	g_pToyPusherTiles[0] = Image_Load32("assets/from-tpb-06-toypusher/1.png");
	g_pToyPusherTiles[1] = Image_Load32("assets/from-tpb-06-toypusher/1b.png");
	g_pToyPusherTiles[2] = Image_Load32("assets/from-tpb-06-toypusher/2.png");
	g_pToyPusherTiles[3] = Image_Load32("assets/from-tpb-06-toypusher/2b.png");
	g_pToyPusherTiles[4] = Image_Load32("assets/from-tpb-06-toypusher/3.png");
	g_pToyPusherTiles[5] = Image_Load32("assets/from-tpb-06-toypusher/3b.png");
	g_pToyPusherTiles[6] = Image_Load32("assets/from-tpb-06-toypusher/4.png");
	g_pToyPusherTiles[7] = Image_Load32("assets/from-tpb-06-toypusher/4b.png");

	for (auto pusher : g_pToyPusherTiles)
		if (nullptr == pusher)
			return false;

	return true;
}

void Shared_Destroy()
{
	for (unsigned iTarget = 0; iTarget < kNumRenderTargets; ++iTarget)
		freeAligned(g_renderTarget[iTarget]);
}
