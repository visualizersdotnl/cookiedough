
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

	// Liveliness
	float WinMidi_GetLiveliness();

	// LFO speed
	float WinMidi_GetLFOSpeed();

	// LFO key sync.
	bool WinMidi_GetLFOSync();

	// Chorus switch
	bool WinMidi_ChorusEnabled();

	// Operator env. rate 
	float WinMidi_GetOpEnvRateMul(unsigned iOp);
	float WinMidi_GetOpEnvRateScale(unsigned iOp);

	// Filter parameters
	float WinMidi_GetFilterCutoff();
	float WinMidi_GetFilterResonance();

	// Pickup distortion
	float WinMidi_GetPickupAmt();
}
