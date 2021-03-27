
// Interlaces clips of the Snatch intro back to PNG files :)

// The ffmpeg-based conversion done in SGP_Q1_Prep is manual.
// The whole bunch is converted to DDS with a command line utility (conv.bat).

// ideas: 
// - use luminosity only and handle color differently
// - ...


// its late im drunk

#include "../code/main.h"
#include "snatchtiler.h"
#include "../code/image.h"

#include "../3rdparty/DevIL-SDK-x64-1.7.8/include/IL/il.h"

// you can tweak this to fit more frames
constexpr unsigned frameX = 512;
constexpr unsigned frameY = 275;

// fits not all frames, but close enough
constexpr unsigned destFrameX = 4096;
constexpr unsigned destFrameY = 4096;

bool Snatchtiler()
{
	const size_t count = 125; // 5 second clips at 25FPS
	std::string basePath1 = "C:/Dev/SGP_Q1_Prep/bin/1/5sec01_00";
	std::string basePath2 = "C:/Dev/SGP_Q1_Prep/bin/2/5sec02_00";
	std::string basePath3 = "C:/Dev/SGP_Q1_Prep/bin/3/5sec03_00";

	std::vector<uint32_t*> frames01;
	std::vector<uint32_t*> frames02;
	std::vector<uint32_t*> frames03;

	// load all 
	for (size_t i = 0; i < count; ++i)
	{
		char number[32];
		sprintf(number, "%03zu", i+1);

		auto path1 = basePath1 + number + ".png";
		auto path2 = basePath2 + number + ".png";
		auto path3 = basePath3 + number + ".png";

		uint32_t *frame01 = Image_Load32(path1);
		uint32_t *frame02 = Image_Load32(path2);
		uint32_t *frame03 = Image_Load32(path3);

		frames01.push_back(frame01);
		frames02.push_back(frame02);
		frames03.push_back(frame03);
	}

	const unsigned xTile = destFrameX/frameX; 
	const unsigned yTile = destFrameY/frameY; 
	
	const unsigned fitframes = xTile*yTile;

	// stash even lines first, then uneven, but for now keep it as is
	uint32_t *dest01 = new uint32_t[destFrameX*destFrameY];
	uint32_t *dest02 = new uint32_t[destFrameX*destFrameY];
	uint32_t *dest03 = new uint32_t[destFrameX*destFrameY]; 
	memset(dest01, 0, destFrameX*destFrameY*4);
	memset(dest02, 1, destFrameX*destFrameY*4);
	memset(dest03, 2, destFrameX*destFrameY*4);

	// write rows of xtile
	for (int i = 0; i < fitframes/xTile; ++i)
	{
		for (unsigned x = 0; x < xTile; ++x)
		{
			size_t srcindex = i*xTile + x;		
			uint32_t *src1 = frames01[srcindex];
			uint32_t *src2 = frames02[srcindex];
			uint32_t *src3 = frames03[srcindex];

			size_t destindex = (i*destFrameX*frameY) + x*frameX;
			uint32_t *dst1 = dest01+destindex;
			uint32_t *dst2 = dest02+destindex;
			uint32_t *dst3 = dest03+destindex;

			for (unsigned yy = 0; yy < frameY; ++yy)
			{
				size_t destindex = yy*destFrameX;
				for (unsigned xx = 0; xx < frameX; ++xx)
				{
					// src pos
					size_t srcindex = yy*frameX + xx;
					dst1[destindex+xx] = src1[srcindex];
					dst2[destindex+xx] = src2[srcindex];
					dst3[destindex+xx] = src3[srcindex];
				}
			}
		}
	}

	ilEnable(IL_FILE_OVERWRITE);

	
	ILuint image;
	ilGenImages(1, &image);
	ilBindImage(image);

	ilTexImage(destFrameX, destFrameY, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, dest01);
	if (0 == ilSaveImage("C:/Dev/SGP_Q1_Prep/bin/1/tiled.png"))
	{
		__debugbreak();
	}

	ilTexImage(destFrameX, destFrameY, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, dest02);
	ilSaveImage("C:/Dev/SGP_Q1_Prep/bin/2/tiled.png");

	ilTexImage(destFrameX, destFrameY, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, dest03);
	ilSaveImage("C:/Dev/SGP_Q1_Prep/bin/3/tiled.png");

	// saves em upside down but who gives a crap

	ilDeleteImages(1, &image);

	delete dest01;
	delete dest02;
	delete dest03;

	return true;

}

// before you continue on this, put dat shit in bonzomatic and go play
