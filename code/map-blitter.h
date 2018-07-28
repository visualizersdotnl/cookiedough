
// cookiedough -- old school 4x4 map interpolation blitters (mainly for computationally heavy effects)

#pragma once

bool MapBlitter_Create();
void MapBlitter_Destroy();

void MapBlitter_Colors(uint32_t* pDest, uint32_t* pSrc);
void MapBlitter_UV(uint32_t* pDest, uint32_t* pSrc, uint32_t* pTexture);
