
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

	// Master
	float WinMidi_GetMasterDrive();

	// Noisyness
	float WinMidi_GetNoisyness();

	// Filter
	float WinMidi_GetFilterDrive();
	float WinMidi_GetFilterAttack();
	float WinMidi_GetFilterDecay();
	float WinMidi_GetFilterSustain();
}
