
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

static void *Image_Load(const std::string &path, bool isGrayscale, unsigned *pNumPixels = nullptr, bool noGC = false)
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
		pPixels = mallocAligned(width*height*sizeof(uint32_t), kAlignTo);
		ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);
		ilCopyPixels(0, 0, 0, width, height, 1, IL_BGRA, IL_UNSIGNED_BYTE, pPixels);
	}
	else
	{
		// load as 8-bit grayscale
		pPixels = mallocAligned(width*height, kAlignTo);
		ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
		ilCopyPixels(0, 0, 0, width, height, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, pPixels);
	}

	ilDeleteImages(1, &image);

	if (false == noGC)
		s_pGC.push_back(pPixels);

	if (nullptr != pNumPixels)
		*pNumPixels = width*height;

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

uint32_t *Image_Load32_CA(const std::string &pathC, const std::string &pathA)
{
	// load color image
	unsigned numPixels;
	uint32_t *pColor = static_cast<uint32_t *>(Image_Load(pathC, false, &numPixels, false));
	if (nullptr == pColor)
		return nullptr;

	// load alpha image (promising to dispose of the pointer if one is returned)
	uint8_t *pAlpha = static_cast<uint8_t *>(Image_Load(pathA, true, nullptr, true));
	if (nullptr == pAlpha)
		return nullptr;

	VIZ_ASSERT(numPixels > 0);

	// combine
	for (unsigned iPixel = 0; iPixel < numPixels; ++iPixel)
	{
		*pColor++ = (*pColor & 0xffffff) | *pAlpha++;
	}

	freeAligned(pAlpha);

	return pColor;
}
