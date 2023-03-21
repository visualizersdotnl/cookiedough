
// cookiedough -- audio: module replay & stream support

#ifndef _AUDIO_H_
#define _AUDIO_H_

#if !defined(_WIN32)
	#define HWND void*
	#include "../3rdparty/bass24-osx/bass.h"
#else
	#include "../3rdparty/bass24-stripped/c/bass.h"
#endif

// 'iDevice' - valid device index or -1 for system default
bool Audio_Create(unsigned int iDevice, const std::string &musicPath, HWND hWnd, bool silent);
void Audio_Destroy();
void Audio_Update();

// for streams: start with this function if not in Rocket sync. editor mode
void Audio_Start_Stream(unsigned bufLenMS);

BASS_INFO &Audio_Get_Info();

// GNU Rocket callbacks
void Audio_Rocket_Pause(void *, int mustPause);
void Audio_Rocket_SetRow(void *, int row);
int Audio_Rocket_IsPlaying(void *);

// use Audio_Rocket_Sync() to get full module position
// feed returned value to Rocket sync_update() as row position!
double Audio_Rocket_Sync(unsigned int &modOrder, unsigned int &modRow, float &modRowAlpha);

// get pos. in seconds (does it work with anything else than streams?)
float Audio_Get_Pos_In_Sec();

#endif // _AUDIO_H_	
