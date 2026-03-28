
// cookiedough -- optimized 32-bit multi-pass (gaussian approximation) blur

#if defined(ARRESTED_DEV_LEGACY)

#ifndef _BOX_BLUR_H_
#define _BOX_BLUR_H_

bool BoxBlur_Create();
void BoxBlur_Destroy();

// use for 'numPasses' if you want a *good* baseline Gaussian blur approximation
constexpr unsigned kGauss = 3;

// important: 
// - buffers must be aligned (use mallocAligned() w/kAlignTo)
// - strength ([0..100]) instead of radius for ease of use with Rocket sync. et cetera (also, this isn't Photoshop)
// - gain ([0..1]) should be left at zero by default, but in case you lose too much power this is a practically free bump

void BoxBlur_Horz32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float strength, float gain, unsigned numPasses);
void BoxBlur_Vert32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float strength, float gain, unsigned numPasses);
void BoxBlur_32(uint32_t *pDest, const uint32_t *pSrc, unsigned xRes, unsigned yRes, float strength, float gain, unsigned numPasses);

#endif // _BOX_BLUR_H_

#endif // ARRESTED_DEV_LEGACY
