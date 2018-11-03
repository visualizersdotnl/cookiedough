
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	- For time being this serves as the physical interface driver.
	- All values are within [0..1] range unless clearly stated otherwise.
	- It should be pretty straightforward to modify the implementation to fit another instrument.
*/

#pragma once

#include "synth-voice.h"

namespace SFM
{
	bool WinMidi_Oxygen49_Start();
	void WinMidi_Oxygen49_Stop();

	/*
		Pull-style values, all normalized [0..1]
	*/

	// Filter
	VoiceFilter WinMidi_GetCurFilter();
	float WinMidi_GetFilterCutoff();
	float WinMidi_GetFilterResonance();
	float WinMidi_GetFilterContour();

	// Feedback
	float WinMidi_GetFeedback();
	float WinMidi_GetFeedbackWetness();
	float WinMidi_GetFeedbackPitch();

	// Pitch bend
	float WinMidi_GetPitchBendRaw(); // [-1..1]
	float WinMidi_GetPitchBend(); // [0..1]

	// Modulation
	float WinMidi_GetModulationIndex();
	float WinMidi_GetModulationRatio();
	float WinMidi_GetModulationBrightness();
	float WinMidi_GetModulationLFOFrequency();

	// ADSR parameters
	float WinMidi_GetAttack();
	float WinMidi_GetDecay();
	float WinMidi_GetSustain();
	float WinMidi_GetRelease();

	// Tremolo
	float WinMidi_GetTremolo();

	// Loop wavetable samples on/off
	bool WinMidi_GetLoopWaves();

	// Algorithm select
	Voice::Algo WinMidi_GetAlgorithm();
}
