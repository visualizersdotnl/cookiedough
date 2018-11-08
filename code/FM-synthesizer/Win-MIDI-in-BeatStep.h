
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

	// Noisyness
	float WinMidi_GetNoisyness();

	// Nintendize
	float WinMidi_GetNintendize();

	// Filter
	float WinMidi_GetFilterAttack();
	float WinMidi_GetFilterDecay();
	float WinMidi_GetFilterSustain();

	// Carrier oscillator
	Waveform WinMidi_GetCarrierOscillator();
	float WinMidi_GetPulseWidth();

	// Algorithm #2
	float WinMidi_GetDoubleDetune();
	float WinMidi_GetDoubleVolume();

	// Algorithm #3
	float WinMidi_GetCarrierVolume1();
	float WinMidi_GetCarrierVolume2();
	float WinMidi_GetCarrierVolume3();
	float WinMidi_GetSlavesLowpass();
	float WinMidi_GetSlavesDetune();
	bool  WinMidi_GetHardSync();
	Waveform WinMidi_GetCarrierOscillator2();
	Waveform WinMidi_GetCarrierOscillator3();
}
