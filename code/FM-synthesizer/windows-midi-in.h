
/*
	Syntherklaas -- FM synthesizer prototype.
	Windows (Win32) MIDI input.

	** Only 1 device at a time! **
	** FIXME: mapping! **
*/

#ifndef _SFM_WINDOWS_MIDI_IN_H_
#define _SFM_WINDOWS_MIDI_IN_H_

namespace SFM
{
	unsigned WinMidi_GetNumDevices();

	bool WinMidi_Start(unsigned iDevice);
	void WinMidi_Stop();

	// FIXME: prototype throw-away code
	float WinMidi_GetTestValue_1();
	float WinMidi_GetTestValue_2();
	float WinMidi_GetTestValue_3();
};

#endif // _SFM_WINDOWS_MIDI_H_
