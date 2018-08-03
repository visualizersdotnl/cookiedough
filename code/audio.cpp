
// cookiedough -- audio: module replay (with BASS)

#include <Windows.h>
#include "main.h"
#include "../3rdparty/bass24-stripped/c/bass.h"
// #include "audio.h"

// FIXME: adjust per module (order also known as pattern)
//        must be a power of 2
const int kRowsPerOrder = 64;

static HMUSIC s_hMusic = NULL;

const DWORD kMusicFlagsProtracker = BASS_MUSIC_PT1MOD|BASS_MUSIC_CALCLEN;
const DWORD kMusicFlagsMisc = BASS_MUSIC_CALCLEN;

bool Audio_Create(unsigned int iDevice, const std::string &musicPath, HWND hWnd)
{
	VIZ_ASSERT(iDevice == -1); // || iDevice < Audio_GetDeviceCount());
	VIZ_ASSERT(hWnd != NULL);

	// BASS device IDs:
	//  0 = No sound (causes functionality to be limited, so -1 is the better pick).
	// -1 = Default.
	// >0 = As enumerated.
	if (!BASS_Init(iDevice, 44100, BASS_DEVICE_LATENCY, hWnd, NULL))
	{ 
		switch (BASS_ErrorGetCode())
		{
		case BASS_ERROR_DEVICE:
		case BASS_ERROR_ALREADY:
		case BASS_ERROR_NO3D:
		case BASS_ERROR_UNKNOWN:
		case BASS_ERROR_MEM:
			VIZ_ASSERT(0);

		case BASS_ERROR_DRIVER:
		case BASS_ERROR_FORMAT:
			SetLastError("Can not initialize BASS audio library @ 44.1 kHz.");
			return false;
		}
	}

	s_hMusic = BASS_MusicLoad(FALSE, (void*)musicPath.c_str(), 0, 0, kMusicFlagsMisc, 0);
	if (NULL == s_hMusic)
	{
		switch (BASS_ErrorGetCode())
		{
		case BASS_ERROR_INIT:
		case BASS_ERROR_NOTAVAIL:
		case BASS_ERROR_ILLPARAM:
		case BASS_ERROR_FILEFORM:
		case BASS_ERROR_FORMAT:
		case BASS_ERROR_MEM:
			VIZ_ASSERT(0);

		case BASS_ERROR_FILEOPEN:
		case BASS_ERROR_UNKNOWN:			
			SetLastError("Can not load music module: " + musicPath);
			return false;
		}
	}

	// maximum precision
	BASS_ChannelSetAttribute(s_hMusic, BASS_ATTRIB_MUSIC_PSCALER, 256); 

	return true;
}

void Audio_Destroy()
{
	BASS_Free();
}

void Audio_Rocket_Pause(void *, int mustPause)
{
	if (mustPause)
		BASS_ChannelPause(s_hMusic);
 	else
		BASS_ChannelPlay(s_hMusic, FALSE);
}

void Audio_Rocket_SetRow(void *, int row)
{
	const int order = row/kRowsPerOrder;
	BASS_ChannelSetPosition(s_hMusic, MAKELONG(order, row&(kRowsPerOrder-1)), BASS_POS_MUSIC_ORDER); 
}

int Audio_Rocket_IsPlaying(void *)
{
	return BASS_ChannelIsActive(s_hMusic);
}

double Audio_Rocket_Sync(unsigned int &modOrder, unsigned int &modRow, float &modRowAlpha)
{
	const QWORD fullPos = BASS_ChannelGetPosition(s_hMusic, BASS_POS_MUSIC_ORDER);
	const DWORD order = LOWORD(fullPos);
	const DWORD row = HIWORD(fullPos)>>8;
	const DWORD rowPart = HIWORD(fullPos)&255;

	modOrder = order;
	modRow = row;
	modRowAlpha = rowPart/256.f;

	return modRowAlpha+(order*kRowsPerOrder + row);
}
