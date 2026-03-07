
// cookiedough -- optimized 32-bit multi-pass (gaussian approximation) blur

#ifndef _BOX_BLUR_H_
#define _BOX_BLUR_H_

bool BoxBlur_Create();
void BoxBlur_Destroy();

// Important: buffers must be aligned (use mallocAligned() w/kAlignTo)
void BoxBlur_Horz32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses);
void BoxBlur_Vert32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses);
void BoxBlur_32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float radius, unsigned numPasses);

#endif // _BOX_BLUR_H_
