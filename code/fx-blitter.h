// cookiedough -- old school 2x2 bilinear blitter plus buffer to use (for heavier effects)

/*
	IMPORTANT:
	- assumes output resolution for blit destination
	- buffers must be 16-byte aligned
*/

#pragma once

constexpr unsigned kFxMapDiv = 2;
constexpr size_t kNumFxMaps = 4;

constexpr size_t kFxMapResX = (kResX/kFxMapDiv)+4; // add 4 pixels guard band for blit implementation (no edge cases)
constexpr size_t kFxMapResY = (kResY/kFxMapDiv)+4; // 4 pixels because effects depend on these being multiples of 4

constexpr size_t kFxMapSize = kFxMapResX*kFxMapResY;
constexpr size_t kFxMapBytes = kFxMapSize*sizeof(uint32_t);

// FIXME: this is just asking for trouble, even for late 1990s standards
extern uint32_t *g_pFxMap[kNumFxMaps];

bool FxBlitter_Create();
void FxBlitter_Destroy();

void Fx_Blit_2x2(uint32_t* pDest, const uint32_t* pSrc);

void FxBlitter_DrawTestPattern(uint32_t* pDest);
