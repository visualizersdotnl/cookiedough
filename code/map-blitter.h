
// cookiedough -- old school 2x2+4x4 map interpolation blitters plus buffers to use (for heavier effects)

// IMPORTANT: assumes output resolution for blit destination!
// IMPORTANT: buffers must be 16-byte aligned

#pragma once

namespace FXMAP
{
	// fine: 2x2
	constexpr unsigned kFineDiv = 2;
	constexpr size_t kFineResX = kResX/kFineDiv;
	constexpr size_t kFineResY = kResY/kFineDiv;
	constexpr size_t kHalfFineX = kFineResX/2;
	constexpr size_t kHalfFineY = kFineResY/2;
	constexpr size_t kFineSize = kFineResX*kFineResY;
	constexpr size_t kFineBytes = kFineSize*sizeof(uint32_t);

	// coarse: 4x4
	constexpr unsigned kCoarseDiv = 4;
	constexpr size_t kCoarseResX = kResX/kCoarseDiv;
	constexpr size_t kCoarseResY = kResY/kCoarseDiv;
	constexpr size_t kHalfCoarseX = kCoarseResX/2;
	constexpr size_t kHalfCoarseY = kCoarseResY/2;
	constexpr size_t kCoarseSize = kCoarseResX*kCoarseResY;
	constexpr size_t kCoarseBytes = kCoarseSize*sizeof(uint32_t);
}

extern uint32_t *g_pFXFine;
extern uint32_t *g_pFXCoarse;

bool MapBlitter_Create();
void MapBlitter_Destroy();

void MapBlitter_Colors_2x2(uint32_t* pDest, uint32_t* pSrc);
void MapBlitter_Colors_2x2_interlaced(uint32_t* pDest, uint32_t* pSrc);
void MapBlitter_Colors_4x4(uint32_t* pDest, uint32_t* pSrc);

// FIXME: implement
// void MapBlitter_UV(...);
// ...
