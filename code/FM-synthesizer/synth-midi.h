
/*
	Syntherklaas -- General MIDI utilities.
*/

#pragma once

#include "synth-single-pole-filters.h"

namespace SFM
{
	// LUT to get frequency for a MIDI key
	extern float g_midiToFreqLUT[127];
	void CalculateMidiToFrequencyLUT(float baseFreq = 440.f);

	// Taken from (though I later added what I needed): https://github.com/FluidSynth/fluidsynth/blob/master/src/midi/fluid_midi.h
	enum MIDI_EventType
	{
		/* Channels */
		CHANNEL_PERCUSSION = 0x9,
		/* Channel messages */
		NOTE_OFF = 0x80,
		NOTE_ON = 0x90,
		KEY_PRESSURE = 0xa0,
		CONTROL_CHANGE = 0xb0,
		PROGRAM_CHANGE = 0xc0,
		CHANNEL_PRESSURE = 0xd0,
		PITCH_BEND = 0xe0,
		/* System exclusive */
		MIDI_SYSEX = 0xf0,
		/* System common - never in MIDI files */
		MIDI_TIME_CODE = 0xf1,
		MIDI_SONG_POSITION = 0xf2,
		MIDI_SONG_SELECT = 0xf3,
		MIDI_TUNE_REQUEST = 0xf6,
		MIDI_EOX = 0xf7,
		/* System real-time - never in MIDI files */
		MIDI_SYNC = 0xf8,
		MIDI_TICK = 0xf9,
		MIDI_START = 0xfa,
		MIDI_CONTINUE = 0xfb,
		MIDI_STOP = 0xfc,
		MIDI_ACTIVE_SENSING = 0xfe,
		MIDI_SYSTEM_RESET = 0xff,
		/* Meta event - for MIDI files only */
		MIDI_META_EVENT = 0xff
	};

	// To smooth out MIDI values if needed (for ex. rotaries)
	class MIDI_Smoothed
	{
	public:
		MIDI_Smoothed(float amount)
		{
			m_LPF.SetCutoff(amount);
		}

		void Set(float value)
		{
			m_LPF.Apply(value);
		}

		float Get() const
		{
			return m_LPF.GetValue();
		}

	private:
		LowpassFilter m_LPF;
	};
}
