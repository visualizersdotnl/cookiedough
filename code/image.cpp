
// cookiedough -- image loader

#include "main.h"

#if !_WIN64
	#include "../3rdparty/DevIL-SDK-x86-1.7.8/include/IL/il.h"
#else
	#include "../3rdparty/DevIL-SDK-x64-1.7.8/include/IL/il.h"
#endif

#include "image.h"

static std::vector<void*> s_pGC;

bool Image_Create()
{
	ilInit();
	s_pGC.clear();

	return true;
}

void Image_Destroy() 
{
	for (auto* pImage : s_pGC)
		freeAligned(pImage);
}

static void *Image_Load(const std::string &path, bool isGrayscale)
{
	ILuint image;
	ilGenImages(1, &image);
	ilBindImage(image);
	
	const ILboolean isLoaded = ilLoadImage(path.c_str());
	if (isLoaded == IL_FALSE)
	{
//		const ILenum error = ilGetError();
		SetLastError("Can not load image: " + path);
		return NULL;
	}

	const ILuint width = ilGetInteger(IL_IMAGE_WIDTH);
	const ILuint height = ilGetInteger(IL_IMAGE_HEIGHT);

	void *pPixels;
	if (!isGrayscale)
	{
		// load as 32-bit ARGB
		pPixels = mallocAligned(width*height*sizeof(uint32_t), kCacheLine);
		ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);
		ilCopyPixels(0, 0, 0, width, height, 1, IL_BGRA, IL_UNSIGNED_BYTE, pPixels);
	}
	else
	{
		// load as 8-bit grayscale
		pPixels = mallocAligned(width*height, kCacheLine);
		ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
		ilCopyPixels(0, 0, 0, width, height, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, pPixels);
	}

	ilDeleteImages(1, &image);

	s_pGC.push_back(pPixels);

	return pPixels;
}

uint32_t *Image_Load32(const std::string &path)
{
	return static_cast<uint32_t *>(Image_Load(path, false));
}

uint8_t *Image_Load8(const std::string &path)
{
	return static_cast<uint8_t *>(Image_Load(path, true));
}
