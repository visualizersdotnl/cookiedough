
// cookiedough -- audio: module replay

#ifndef _AUDIO_H_
#define _AUDIO_H_

// 'iDevice' - valid device index or -1 for system default
bool Audio_Create(unsigned int iDevice, const std::string &musicPath, HWND hWnd, bool silent);
void Audio_Destroy();

// GNU Rocket callbacks
void Audio_Rocket_Pause(void *, int mustPause);
void Audio_Rocket_SetRow(void *, int row);
int Audio_Rocket_IsPlaying(void *);

// use Audio_Rocket_Sync() to get full module position
// feed returned value to Rocket sync_update() as row position!
double Audio_Rocket_Sync(unsigned int &modOrder, unsigned int &modRow, float &modRowAlpha);

#endif // _AUDIO_H_	
