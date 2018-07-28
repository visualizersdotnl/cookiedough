
// cookiedough -- old school 4x4 map interpolation blitters (mainly for computationally heavy effects)

#pragma once

const size_t kFXMapResX = kResX/4;
const size_t kFXMapResY = kResY/4;
constexpr size_t kHalfFXMapResX = kFXMapResX/2;
constexpr size_t kHalfFXMapResY = kFXMapResY/2;
constexpr size_t kFXMapSize = kFXMapResX*kFXMapResY;
constexpr size_t kFXMapBytes = kFXMapSize*sizeof(uint32_t); // FIXME: will this do for UVs?

bool MapBlitter_Create();
void MapBlitter_Destroy();

// FIXME: assumes output resolution, might be a problem if shared render target is smaller or bigger
void MapBlitter_Colors(uint32_t* pDest, uint32_t* pSrc);
// void MapBlitter_UV(...);
