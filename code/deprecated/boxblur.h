
// cookiedough -- deprecated 2007 box blur implementation 

#if defined(ARRESTED_DEV_LEGACY)

#ifndef _BOX_BLUR_H_DEPRECATED
#define _BOX_BLUR_H_DEPRECATED

// use [1..100] and multiply it by this value to feed as 'strength'
// this way you can conveniently define no blur as zero in Rocket and use [1..100] to define strength a bit more intuitively
constexpr float kBoxBlurScale = 0.01f;

CKD_INLINE static float BoxBlurScale(float strength) 
{
	if (strength != 0.f)
		strength = clampf(1.f, 100.f, strength)*kBoxBlurScale;

	return strength;
}

void HorizontalBoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength);

void VerticalBoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength);

// full 2D blur
void BoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength);

#endif // _BOX_BLUR_H_DEPRECATED

#endif // ARRESTED_DEV_LEGACY
