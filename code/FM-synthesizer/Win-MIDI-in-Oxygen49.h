
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	- For time being this serves as the physical interface driver.
	- All values are within [0..1] range; rotaries and faders are interpolated.
	- It should be pretty straightforward to modify the implementation to fit another instrument.
*/

#pragma once

namespace SFM
{
	unsigned WinMidi_GetNumDevices();

	bool WinMidi_Start(unsigned iDevice);
	void WinMidi_Stop();

	/*
		Pull-style values, all normalized [0..1]
	*/

	// Filter
	float WinMidi_GetFilterCutoff();
	float WinMidi_GetFilterResonance();
	float WinMidi_GetFilterWetness();
	float WinMidi_GetFilterEnvInfl();

	// Feedback
	float WinMidi_GetFeedback();
	float WinMidi_GetFeedbackWetness();
	float WinMidi_GetFeedbackPitch();

	// Master drive & pitch bend
	float WinMidi_GetMasterDrive();
	float WinMidi_GetPitchBendRaw(); // [-1..1]
	float WinMidi_GetPitchBend(); // [0..1]

	// Modulation main
	float WinMidi_GetModulationIndex();
	float WinMidi_GetModulationRatio();
	float WinMidi_GetModulationBrightness();

	// Modulation LFO
	float WinMidi_GetModulationLFOShape();
	float WinMidi_GetModulationLFOFrequency();
	float WinMidi_GetModulationLFOPower();

	// ADSR parameters
	float WinMidi_GetAttack();
	float WinMidi_GetDecay();
	float WinMidi_GetSustain();
	float WinMidi_GetRelease();
}
