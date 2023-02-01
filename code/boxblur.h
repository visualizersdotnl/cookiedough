
// cookiedough -- box blur filter (32-bit only, for now)

#ifndef _BOX_BLUR_H_
#define _BOX_BLUR_H_

// use [1..100] and multiply it by this value to feed as 'strength'
// this way you can conveniently define no blur as zero in Rocket and use [1..100] to define strength a bit more intuitively
constexpr float kBoxBlurScale = 0.005f;

// some more observation(s):
// - combine either blur with a polar transformation to get, for example, a radial blur

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

#endif // _BOX_BLUR_H_
