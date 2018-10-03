
/*
	Syntherklaas FM
	(C) syntherklaas.org, a subsidiary of visualizers.nl

	Uses a little hack in the BASS layer of this codebase (audio.cpp) to output a 44.1kHz monaural stream.
*/

#ifndef _SFM_H_
#define _SFM_H_

#include "global.h"

bool Syntherklaas_Create();
void Syntherklaas_Destroy();
void Syntherklaas_Render(uint32_t *pDest, float time, float delta);

// To feed BASS (see audio.h):
#include "../audio.h"
DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser);

#endif
