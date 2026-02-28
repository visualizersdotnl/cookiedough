
// cookiedough -- audio: module replay (with BASS)

#include "main.h"
// #include "../3rdparty/bass24-stripped/c/bass.h"
#include "audio.h"

#if defined(_WIN32)
	#include <Windows.h>
#endif

// const int kRowsPerOrder = 64; // FIXME: adjust per module (order also known as pattern); must be a power of 2
// const DWORD kMusicFlagsProtracker = BASS_MUSIC_PT1MOD|BASS_MUSIC_CALCLEN;
// const DWORD kMusicFlagsMisc = BASS_MUSIC_CALCLEN;

// MP3/OGG sync. rate
// HACK: we're compensating for the fact that it's actually 170BPM here instead of 174BPM, without wanting to resync. everything
const double kRowRate = (170.0 /* BPM */ / (60.0*(170.0/174.0)))*16.0 /* RPB */;

static HMUSIC s_hMusic = 0;
static BASS_INFO s_bassInf;

bool Audio_Create(unsigned int iDevice, const std::string &musicPath, HWND hWnd, bool silent)
{
	VIZ_ASSERT(iDevice == unsigned(-1)); // || iDevice < Audio_GetDeviceCount());

#if defined(_WIN32)
	VIZ_ASSERT(hWnd != NULL);
#endif

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

	BASS_GetInfo(&s_bassInf);

	/* for modules; patch up if/when necessary:
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
	*/

	const DWORD streamFlags = BASS_SAMPLE_FX | BASS_MP3_SETPOS | BASS_STREAM_PRESCAN | ( (0 == iDevice) ? BASS_STREAM_DECODE : 0 );
	s_hMusic = BASS_StreamCreateFile(FALSE, musicPath.c_str(), 0, 0, streamFlags /* BASS_UNICODE */);
	if (s_hMusic == 0)
	{
		switch (BASS_ErrorGetCode())
		{
		case BASS_ERROR_INIT:
		case BASS_ERROR_NOTAVAIL:
		case BASS_ERROR_ILLPARAM:
		case BASS_ERROR_NO3D:
		case BASS_ERROR_FILEFORM:
		case BASS_ERROR_CODEC:
		case BASS_ERROR_FORMAT:
		case BASS_ERROR_SPEAKER:
		case BASS_ERROR_MEM:
			VIZ_ASSERT(0);

		case BASS_ERROR_FILEOPEN:
		case BASS_ERROR_UNKNOWN:			
			SetLastError("Can not load MP3/OGG: " + musicPath);
			return false;
		}
	}

	if (silent)
		BASS_ChannelSetAttribute(s_hMusic, BASS_ATTRIB_VOL, 0.f);

	return true;
}

void Audio_Destroy()
{
	BASS_Free();
}

void Audio_Update() {}

void Audio_Start_Stream(unsigned bufLenMS)
{
	BASS_ChannelPlay(s_hMusic, TRUE);
}

bool Audio_Check_Stream()
{
	const DWORD result = BASS_ChannelIsActive(s_hMusic);
	if (BASS_ACTIVE_STALLED == result)
	{
		VIZ_ASSERT(false);
		return false;
	}

	return true;
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
//	for modules; patch up if/when necessary:
//	const int order = row/kRowsPerOrder;
//	BASS_ChannelSetPosition(s_hMusic, MAKELONG(order, row&(kRowsPerOrder-1)), BASS_POS_MUSIC_ORDER); 

	VIZ_ASSERT(s_hMusic != 0);
	const double secPos = row/kRowRate;
	const QWORD newChanPos = BASS_ChannelSeconds2Bytes(s_hMusic, secPos);
	BASS_ChannelSetPosition(s_hMusic, newChanPos, BASS_POS_BYTE);
}

int Audio_Rocket_IsPlaying(void *)
{
	return BASS_ChannelIsActive(s_hMusic);
}

double Audio_Rocket_Sync(unsigned int &modOrder, unsigned int &modRow, float &modRowAlpha)
{
/*  for modules; patch up when/if necessary:

	const QWORD fullPos = BASS_ChannelGetPosition(s_hMusic, BASS_POS_MUSIC_ORDER);
	const DWORD order = LOWORD(fullPos);
	const DWORD row = HIWORD(fullPos)>>8;
	const DWORD rowPart = HIWORD(fullPos)&255;

	modOrder = order;
	modRow = row;
	modRowAlpha = rowPart/256.f;

	return modRowAlpha+(order*kRowsPerOrder + row);
*/

	VIZ_ASSERT(s_hMusic != 0);
	const QWORD chanPos = BASS_ChannelGetPosition(s_hMusic, BASS_POS_BYTE);
	const double secPos = BASS_ChannelBytes2Seconds(s_hMusic, chanPos);
	return secPos*kRowRate;
}

float Audio_Get_Pos_In_Sec()
{
	VIZ_ASSERT(s_hMusic != 0);
	const QWORD chanPos = BASS_ChannelGetPosition(s_hMusic, BASS_POS_BYTE);
	const double secPos = BASS_ChannelBytes2Seconds(s_hMusic, chanPos);
	return float(secPos);
}