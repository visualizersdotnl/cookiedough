
/*
	Syntherklaas FM
	(C) syntherklaas.org, a subsidiary of visualizers.nl

	Uses a little hack in the BASS layer of this codebase (audio.cpp) to output a 44.1kHz monaural stream.
*/

#ifndef _FM_BISON_H_
#define _FM_BISON_H_

#include "synth-global.h"

bool Syntherklaas_Create();
void Syntherklaas_Destroy();
void Syntherklaas_Render(uint32_t *pDest, float time, float delta);

namespace SFM
{
	/*
		API exposed to MIDI input.
	*/

	// Trigger a note (if possible) and return it's internal index
	unsigned TriggerNote(unsigned midiIndex);

	// Release a note using it's internal index
	void ReleaseNote(unsigned index);
}

// To feed BASS (see audio.h):
#include "../audio.h"
DWORD CALLBACK Syntherklaas_StreamFunc(HSTREAM hStream, void *pDest, DWORD length, void *pUser);

#endif // _FM_BISON_H_
