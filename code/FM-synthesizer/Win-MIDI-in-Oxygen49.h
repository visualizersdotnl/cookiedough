
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

	// Pull-style values:

	// Filter
	float WinMidi_GetFilterCutoff();
	float WinMidi_GetFilterResonance();
	float WinMidi_GetFilterWetness();

	// Master
	float WinMidi_GetMasterDrive();
	float WinMidi_GetMasterModulationIndex();
}
