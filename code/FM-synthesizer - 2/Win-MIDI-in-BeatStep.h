
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the Arturia BeatStep; an addition to the Oxygen 49.
*/

#pragma once

#include "synth-oscillator.h"

namespace SFM
{
	bool WinMidi_BeatStep_Start();
	void WinMidi_BeatStep_Stop();

	/*
		Pull-style values, all normalized [0..1]
	*/

	// Vibrato & operator vibrato control
	float WinMidi_GetVibrato();
	float WinMidi_GetOperatorVibrato();

	// Delay
	float WinMidi_GetDelayWet();
	float WinMidi_GetDelayRate();
	float WinMidi_GetDelayWidth();
	float WinMidi_GetDelayFeedback();

	// Filter
	float WinMidi_GetFilterA();
	float WinMidi_GetFilterD();
	float WinMidi_GetFilterS();

	// Note jitter
	float WinMidi_GetNoteJitter();
}
