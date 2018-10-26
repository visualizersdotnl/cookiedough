
/*
	Syntherklaas -- General MIDI utilities.
*/

#pragma once

#include "3rdparty/juce_LinearSmoothedValue.h"

namespace SFM
{
	// Lookup table to translate any MIDI key to a frequency (see implementation for base)
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

	// For output range [0..1]
	class MIDI_Smoothed
	{
	public:
		MIDI_Smoothed(float value = 0.f, unsigned range = 127) :
			m_range(range)
,			m_value(value)
		{
			m_value.reset(kSampleRate, 0.025f);
		}

	private:
		const unsigned m_range;
		juce::LinearSmoothedValue<float> m_value;

	public:
		void Set(unsigned iValue)
		{
			const float value = float(iValue)/m_range;
			m_value.setValue(value);
		}

		float Get() /* const */
		{
			return Clamp(m_value.getNextValue());
		}
	};
}
