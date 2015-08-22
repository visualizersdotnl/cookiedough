
// cookiedough -- image loader

#ifndef _IMAGE_H_
#define _IMAGE_H_

bool Image_Create();
void Image_Destroy();

// important: caller assumes ownership of pointer (use delete[])
uint32_t *Image_Load32(const std::string &path);
uint8_t *Image_Load8(const std::string &path);

#endif // _IMAGE_H_
