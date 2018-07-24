
// cookiedough -- audio: module replay (with BASS)

#include <Windows.h>
#include "main.h"
#include "../3rdparty/bassmod20/c/bassmod.h"
// #include "audio.h"

bool Audio_Create(unsigned int iDevice, const std::string &musicPath, HWND hWnd)
{
	VIZ_ASSERT(iDevice == -1); // || iDevice < Audio_GetDeviceCount());
	VIZ_ASSERT(hWnd != NULL);

	// correct DLL version?
	if (BASSMOD_GetVersion() != MAKELONG(2,0))
	{
		SetLastError("Incorrect version of BASSMOD DLL loaded.");
		return false;
	}

	// BASS device IDs:
	//  0 = No sound (causes functionality to be limited, so -1 is the better pick).
	// -1 = Default.
	// >0 = As enumerated.
	if (!BASSMOD_Init((iDevice == -1) ? -1 : iDevice + 1, 44100, BASS_DEVICE_MONO))
	{ 
		switch (BASSMOD_ErrorGetCode())
		{
		case BASS_ERROR_DEVICE:
		case BASS_ERROR_ALREADY:
		case BASS_ERROR_UNKNOWN:
		case BASS_ERROR_MEM:
			VIZ_ASSERT(0);

		case BASS_ERROR_DRIVER:
		case BASS_ERROR_FORMAT:
			SetLastError("Can not initialize BASS audio library @ 44.1 kHz.");
			return false;
		}
	}

	if (FALSE == BASSMOD_MusicLoad(FALSE, (void*)musicPath.c_str(), 0, 0, BASS_MUSIC_PT1MOD|BASS_MUSIC_CALCLEN))
	{
		switch (BASSMOD_ErrorGetCode())
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

	// timing precision
	BASSMOD_MusicSetPositionScaler(256);

	return true;
}

void Audio_Destroy()
{
	BASSMOD_Free();
}

void Audio_Rocket_Pause(void *, int mustPause)
{
	if (mustPause)
		BASSMOD_MusicPause();
	else
		BASSMOD_MusicPlay();	
}

void Audio_Rocket_SetRow(void *, int row)
{
	BASSMOD_MusicSetPosition(row>>6 | (row&63)<<16);
}

int Audio_Rocket_IsPlaying(void *)
{
	return BASSMOD_MusicIsActive();
}

int Audio_Rocket_Sync(unsigned int &modOrder, unsigned int &modRow, float &modRowAlpha)
{
	const DWORD fullPos = BASSMOD_MusicGetPosition();
	const DWORD order = LOWORD(fullPos);
	const DWORD row = HIWORD(fullPos)/256;
	const DWORD rowPart = HIWORD(fullPos)%256;

	modOrder = order;
	modRow = row;
	modRowAlpha = rowPart/256.f;

	return order*64 + row; // FIXME
}
