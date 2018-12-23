
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	- For time being this serves as the (hardcoded) physical interface driver, with the BeatStep counterpart.
	- All values are within [0..1] range unless clearly stated otherwise.
	- It should be pretty straightforward to modify the implementation to fit another instrument.
*/

#pragma once

#include "synth-parameters.h"
#include "synth-stateless-oscillators.h"

namespace SFM
{
	bool WinMidi_Oxygen49_Start();
	void WinMidi_Oxygen49_Stop();

	// ** HACK: BeatStep also wants to know the current operator, for now **
	extern unsigned g_currentOp;

	/*
		Pull-style values, all normalized [0..1]
	*/

	// Master
	float WinMidi_GetMasterDrive();

	// Tremolo
	float WinMidi_GetTremolo();

	// Pitch bend
	float WinMidi_GetPitchBend();

	// Modulation: vibrato or if editing operators: modulation depth
	float WinMidi_GetModWheel();

	// Operator control
	bool  WinMidi_GetOperatorFixed();
	float WinMidi_GetOperatorCoarse();
	float WinMidi_GetOperatorFinetune();
	float WinMidi_GetOperatorDetune();
	float WinMidi_GetOperatorAmplitude();
	float WinMidi_GetOperatorTremolo();
	float WinMidi_GetOperatorFeedbackAmount();
	float WinMidi_GetOperatorVelocitySensitivity();
	float WinMidi_GetOperatorPitchEnvAmount();
	float WinMidi_GetOperatorLevelScaleL();
	float WinMidi_GetOperatorLevelScaleR();

	// [0..127] MIDI
	unsigned WinMidi_GetOperatorLevelScaleBP(); 

	// Current operator to apply new values to (-1 if none)
	unsigned WinMidi_GetOperator();

	// Master ADSR
	float WinMidi_GetAttack();
	float WinMidi_GetDecay();
	float WinMidi_GetSustain();
	float WinMidi_GetRelease();

	// Operator envelope (attack (level) & decay)
	float WinMidi_GetOperatorEnvA();
	float WinMidi_GetOperatorEnvL();
	float WinMidi_GetOperatorEnvD();

	// LFO shape
	Waveform WinMidi_GetLFOShape();

	// Filter
	int   WinMidi_GetFilterType();
	float WinMidi_GetCutoff();
	float WinMidi_GetResonance();
	float WinMidi_GetFilterWet(); // Also known as 'contour'
	bool  WinMidi_GetFilterInv(); // Invert envelope value ([0..-1] instead of [0..1])

	// Pitch env. invert (like WinMidi_GetFilterInv())
	int WinMidi_GetOperatorPitchEnvInv();
}
