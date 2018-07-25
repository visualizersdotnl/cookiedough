
// cookiedough -- image loader

#ifndef _IMAGE_H_
#define _IMAGE_H_

bool Image_Create();
void Image_Destroy();

uint32_t *Image_Load32(const std::string &path);
uint8_t *Image_Load8(const std::string &path);

void Image_Free(void *pImage);

#endif // _IMAGE_H_
