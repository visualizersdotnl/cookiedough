
// cookiedough -- old school 2x2 interpolating blitter plus buffer to use (for heavier effects)

/*
	IMPORTANT:
	- assumes output resolution for blit destination
	- buffers must be 16-byte aligned

	FIXME:
	- test more, it's 23:20 and I'm tired
	- missing a pixel on the side and on the bottom
*/

#pragma once

constexpr unsigned kFxMapDiv = 2;

constexpr size_t kFxMapResX = kResX/kFxMapDiv;
constexpr size_t kFxMapResY = kResY/kFxMapDiv;

constexpr size_t kFxMapSize = kFxMapResX*kFxMapResY;
constexpr size_t kFxMapBytes = kFxMapSize*sizeof(uint32_t);

// useful to have a few of these for stuff like a blur pass that likes a separate dest. buffer
extern uint32_t *g_pFxMap[4];

bool FxBlitter_Create();
void FxBlitter_Destroy();

void Fx_Blit_2x2(uint32_t* pDest, uint32_t* pSrc);

void FxBlitter_DrawTestPattern(uint32_t* pDest);