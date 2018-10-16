
/*
	Syntherklaas -- Generic MIDI-related.
*/

#ifndef _SFM_SYNTH_MIDI_H_
#define _SFM_SYNTH_MIDI_H_

namespace SFM
{
	// Lookup table to translate any MIDI key to a frequency, with 440Hz as the base.
	void CalcMidiToFreqLUT();
	extern float g_midiToFreqLUT[127];

	// Stolen from: https://github.com/FluidSynth/fluidsynth/blob/master/src/midi/fluid_midi.h
	enum MIDI_EventType
	{
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

	// FIXME: interpolate
	class MIDI_Smoothed
	{
	public:
		MIDI_Smoothed() :
			m_timeStamp(0), m_value(0.f) {}

	private:
		unsigned m_timeStamp;
		float m_value;

	public:
		void Set(unsigned iValue, unsigned timeStamp)
		{
			m_value = iValue/127.f;
		}

		float Get() const
		{
			return m_value;
		}
	};
}

#endif // _SFM_SYNTH_MIDI_H_
