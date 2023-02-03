
// cookiedough -- old Javeline-era box blur filter (32-bit only, for now)

#ifndef _BOX_BLUR_H_
#define _BOX_BLUR_H_

bool BoxBlur_Create();
void BoxBlur_Destroy();

// use [1..100] and multiply it by this value to feed as 'strength'
// this way you can conveniently define no blur as zero in Rocket and use [1..100] to define strength a bit more intuitively
constexpr float kBoxBlurScale = 0.01f;

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

#endif // _BOX_BLUR_H_
