
/*
	Syntherklaas FM -- Windows (Win32) MIDI input, explicitly designed (for now) for the M-AUDIO Oxygen 49.
	All values are within [0..1] range; rotaries and faders are interpolated.
*/

#ifndef _SFM_WINDOWS_MIDI_IN_H_
#define _SFM_WINDOWS_MIDI_IN_H_

namespace SFM
{
	unsigned WinMidi_GetNumDevices();

	bool WinMidi_Start(unsigned iDevice);
	void WinMidi_Stop();

	float WinMidi_GetCutoff();
	float WinMidi_GetResonance();
	float WinMidi_GetFilterMix();
};

#endif // _SFM_WINDOWS_MIDI_H_
