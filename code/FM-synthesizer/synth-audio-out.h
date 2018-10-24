
/*
	Syntherklaas FM - SDL2 audio output driver.
*/

#pragma once

// To feed Bevacqua's audio stream (hack):
#include "../audio.h"
DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser);
