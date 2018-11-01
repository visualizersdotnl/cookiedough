
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the Arturia BeatStep; an addition to the Oxygen 49.
*/

#pragma once

#include "synth-voice.h"

namespace SFM
{
	bool WinMidi_BeatStep_Start();
	void WinMidi_BeatStep_Stop();

	/*
		Pull-style values, all normalized [0..1]
	*/

	// Filter
	float WinMidi_GetFilterDrive() ; // MOOG 1974's emphasis parameter
	float WinMidi_GetFilterAttack();
	float WinMidi_GetFilterDecay();
	float WinMidi_GetFilterSustain();
}
