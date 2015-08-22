
// cookiedough -- box blur filter (32-bit only, for now)

#ifndef _BOX_BLUR_H_
#define _BOX_BLUR_H_

void HorizontalBoxBlur32(
	uint32_t *pDest,
	const uint32_t *pSrc,
	unsigned int xRes,
	unsigned int yRes,
	float strength);

#endif // _BOX_BLUR_H_
