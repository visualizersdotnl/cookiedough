
// cookiedough -- polar blits (640x480 -> kResX*kResY)

#ifndef _POLAR_H_
#define _POLAR_H_

bool Polar_Create();
void Polar_Destroy();

void Polar_Blit(const uint32_t *pSrc, uint32_t *pDest, bool inverse = false);

#endif // _POLAR_H_
