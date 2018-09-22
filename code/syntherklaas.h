
/*
	Syntherklaas -- FM synthesizer prototype.

	Riding along with this codebase because it's a great testbed.
	Uses a little hack in my BASS layer (audio.cpp) to output to a 44100 Hz monaural stream.
*/

#pragma once

bool Syntherklaas_Create();
void Syntherklaas_Destroy();
void Syntherklaas_Render(uint32_t *pDest, float time, float delta);

// To feed BASS (see audio.h):
#include "audio.h"
DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser);
