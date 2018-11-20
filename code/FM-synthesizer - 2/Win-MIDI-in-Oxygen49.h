
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	- For time being this serves as the (hardcoded) physical interface driver, with the BeatStep counterpart.
	- All values are within [0..1] range unless clearly stated otherwise.
	- It should be pretty straightforward to modify the implementation to fit another instrument.
*/

#pragma once

#include "synth-parameters.h"

namespace SFM
{
	bool WinMidi_Oxygen49_Start();
	void WinMidi_Oxygen49_Stop();

	/*
		Pull-style values, all normalized [0..1]
	*/

	// Master
	float WinMidi_GetMasterDrive();
	float WinMidi_GetVibrato();

	// Pitch bend
	float WinMidi_GetPitchBend();

	// Modulation (depth)
	float WinMidi_GetModulation();

	// Operator control
	float WinMidi_GetOperatorCoarse();
	float WinMidi_GetOperatorFinetune();
	float WinMidi_GetOperatorDetune();
	float WinMidi_GetOperatorAmplitude();
	float WinMidi_GetOperatorVibrato();

	// Current operator to set new values to (-1 if none)
	unsigned WinMidi_GetOperator();

	// Master ADSR
	float WinMidi_GetAttack();
	float WinMidi_GetDecay();
	float WinMidi_GetSustain();
	float WinMidi_GetRelease();

	// Modulation envelope (attack & decay)
	float WinMidi_GetModEnvA();
	float WinMidi_GetModEnvD();

	// Filter
	float WinMidi_GetCutoff();
	float WinMidi_GetResonance();
	float WinMidi_GetFilterWet();

	// Note jitter
	float WinMidi_GetNoteJitter();
}
