
/*
	Syntherklaas FM - Audio output driver.

	FIXME:
		- Use SDL2 first
		- Later optimize for target hardware, retaining SDL2 output
*/

#pragma once

// To feed Bevacqua's audio stream (hack):
#include "../audio.h"
DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser);
