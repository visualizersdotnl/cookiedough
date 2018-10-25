
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
	float WinMidi_GetFeedbackPhaser();

	// Master drive
	float WinMidi_GetMasterDrive();

	// Master modulation main
	float WinMidi_GetMasterModulationIndex();
	float WinMidi_GetMasterModulationRatio();
	float WinMidi_GetMasterModulationUnused();

	// Master modulation LFO
	float WinMidi_GetMasterModLFOShape();
	float WinMidi_GetMasterModLFOFrequency();
	float WinMidi_GetMasterModLFOPower();
	float WinMidi_GetMasterModBrightness();

	// Master ADSR parameters
	float WinMidi_GetMasterAttack();
	float WinMidi_GetMasterDecay();
	float WinMidi_GetMasterSustain();
	float WinMidi_GetMasterRelease();
}
