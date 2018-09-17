
// cookiedough -- polar blits

#ifndef _POLAR_H_
#define _POLAR_H_

bool Polar_Create();
void Polar_Destroy();

// render target to output resolution
void Polar_Blit(const uint32_t *pSrc, uint32_t *pDest, bool inverse = false);

// for 2x2 effect map
void Polar_Blit_2x2(const uint32_t *pSrc, uint32_t *pDest, bool inverse = false);

#endif // _POLAR_H_
