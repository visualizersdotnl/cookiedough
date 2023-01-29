
// cookiedough -- this is where functional tests go

#include "main.h"
// #include "tests.h"
#include "bilinear.h"

static uint32_t s_tile[2*2];

bool RunTests()
{
	// test bilinear samplers (32-bit)
	s_tile[0] =  0;
	s_tile[1] = -1;
	s_tile[2] =  0;
	s_tile[3] = -1;

	auto colorA_16 = v2cISSE16(bsamp32_16(s_tile, 0, 0, 1, 1,   0*0x010101,   0*0x010101));
	auto colorB_16 = v2cISSE16(bsamp32_16(s_tile, 0, 0, 1, 1, 255*0x010101, 255*0x010101));
	auto colorC_16 = v2cISSE16(bsamp32_16(s_tile, 0, 0, 1, 1, 127*0x010101, 127*0x010101));
	if (colorA_16 != s_tile[0] || colorB_16 != s_tile[3] && colorC_16 == 0x7f7f7f)
	{
		SetLastError("bsamp32_16() broken!");
		return false;
	}

	auto colorA_32 = v2cISSE32(bsamp32_32(s_tile, 0, 0, 1, 1,   0*0x010101,   0*0x010101));
	auto colorB_32 = v2cISSE32(bsamp32_32(s_tile, 0, 0, 1, 1, 255*0x010101, 255*0x010101));
	auto colorC_32 = v2cISSE32(bsamp32_32(s_tile, 0, 0, 1, 1, 127*0x010101, 127*0x010101));
	if (colorA_32 != s_tile[0] || colorB_32 != s_tile[3] || colorC_32 != 0x7f7f7f)
	{
		SetLastError("bsamp32_32() broken!");
		return false;
	}

	return true;
}
