
/*
	Syntherklaas FM -- Windows (Win32) MIDI input, explicitly designed (for now) for the M-AUDIO Oxygen 49.
	All values are within [0..1] range; rotaries and faders are interpolated.
*/

#include "synth-global.h"
#include "windows-midi-in.h"
#include "synth-midi.h"
#include "FM_BISON.h"

// Bevacqua mainly relies on SDL + standard C(++), so:
#pragma comment(lib, "Winmm.lib")
#include <Windows.h>
#include <Mmsystem.h>

namespace SFM
{
	static HMIDIIN s_hMidiIn = NULL;

	// FIXME: not yet used, but should be!
	static const size_t kMidiBufferLength = 256;
	static uint8_t s_buffer[kMidiBufferLength];

	static MIDIHDR s_header;

	SFM_INLINE unsigned MsgType(unsigned parameter)   { return parameter & 0xf0; }
	SFM_INLINE unsigned MsgChan(unsigned parameter)   { return parameter & 0x0f; }
	SFM_INLINE unsigned MsgParam1(unsigned parameter) { return (parameter>>8)  & 0x7f; }
	SFM_INLINE unsigned MsgParam2(unsigned parameter) { return (parameter>>16) & 0x7f; }

	// Mapping: 3 rotaries
	const unsigned kPotCutoff = 22;
	const unsigned kPotResonance = 23;
	const unsigned kPotFilterMix = 26;
	MIDI_Smoothed s_cutoff, s_resonance, s_filterMix;

	// Mapping: 49 keys
	const unsigned kUpperKey = 36;
	const unsigned kLowerKey = 84;

	SFM_INLINE bool IsKey(unsigned index) 
	{ 
		return index >= kUpperKey && index <= kLowerKey; 
	}

	// Array can be smaller (FIXME)
	static unsigned s_voices[127];

	static void WinMidiProc(
		HMIDI hMidiIn,
		UINT wMsg,
		DWORD_PTR dwInstance,
		DWORD_PTR _dwParam1, 
		DWORD_PTR _dwParam2)
	{
		// Screw 32-bit warnings
		unsigned dwParam1 = (unsigned) _dwParam1; // Information
		unsigned dwParam2 = (unsigned) _dwParam2; // Time stamp

		switch (wMsg)
		{
		case MIM_DATA:
		{
			unsigned eventType = MsgType(dwParam1);
			unsigned channel = MsgChan(dwParam1);
			unsigned controlIdx = MsgParam1(dwParam1);
			unsigned controlVal = MsgParam2(dwParam1);

			// Dumps incoming events, very useful
			static char buffer[128];
			sprintf(buffer, "MIDI input: Type %u Chan %u Idx %u Val %u Time %u", eventType, channel, controlIdx, controlVal, dwParam2);
			Log(buffer);

			switch (eventType)
			{
			default:
				{
					// Rotaries (FIXME: code repetition)
					if (kPotCutoff == controlIdx)
					{
						s_cutoff.Set(controlVal, dwParam2);
					}
					else if (kPotResonance == controlIdx)
					{
						s_resonance.Set(controlVal, dwParam2);
					}
					else if (kPotFilterMix == controlIdx)
					{
						s_filterMix.Set(controlVal, dwParam2);
					}

					break;
				}

			case PITCH_BEND:
				{
					unsigned bend = (controlVal<<7)|controlIdx;
					break;
				}

			case NOTE_ON:
				{
					SFM_ASSERT(true == IsKey(controlIdx));
					const unsigned iVoice = TriggerNote(g_midiToFreqLUT[controlIdx]);
					s_voices[controlIdx] = iVoice;
					break;
				}

			case NOTE_OFF:
				{
					SFM_ASSERT(true == IsKey(controlIdx));
					const unsigned iVoice = s_voices[controlIdx];
					if (-1 != iVoice)
					{
						ReleaseVoice(iVoice);
						s_voices[controlIdx] = -1;
					}

					break;
				}
			}
			
			return;
		}

		case MIM_LONGDATA:
			// FIXE: implement!
			Log("Implement MIM_LONGDATA!");
			break;

		// Handled
		case MIM_OPEN:
		case MIM_CLOSE:
			break; 

		// Shouldn't happen, but let's assert
		case MIM_ERROR:
		case MIM_LONGERROR:
		case MIM_MOREDATA:
			SFM_ASSERT(false);
			break;

		default:
			SFM_ASSERT(false);
		}
	}

	unsigned WinMidi_GetNumDevices()
	{
		return midiInGetNumDevs();
	}

	bool WinMidi_Start(unsigned devIdx = 0)
	{
		if (WinMidi_GetNumDevices() < devIdx)
		{
			SFM_ASSERT(false);
			return false;
		}

		MIDIINCAPS devCaps;
		const MMRESULT result = midiInGetDevCaps(devIdx, &devCaps, sizeof(MIDIINCAPS));
		SFM_ASSERT(MMSYSERR_NOERROR == result); 
		Log(devCaps.szPname);

		const auto openRes = midiInOpen(&s_hMidiIn, devIdx, (DWORD_PTR) WinMidiProc, NULL, CALLBACK_FUNCTION);
		if (MMSYSERR_NOERROR == openRes)
		{
			s_header.lpData = (LPSTR) s_buffer; // I'm not going to attempt to make this nonsene prettier.
			s_header.dwBufferLength = kMidiBufferLength;
			s_header.dwFlags = 0;

			const auto hdrPrepRes = midiInPrepareHeader(s_hMidiIn, &s_header, sizeof(MIDIHDR));
			if (MMSYSERR_NOERROR == hdrPrepRes)
			{
				const auto queueRes = midiInAddBuffer(s_hMidiIn, &s_header, sizeof(MIDIHDR));
				if (MMSYSERR_NOERROR == queueRes)
				{
					// Reset voice indices (to -1)
					memset(s_voices, 0xff, 127*sizeof(unsigned));

					const auto startRes = midiInStart(s_hMidiIn);
					if (MMSYSERR_NOERROR == startRes)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	void WinMidi_Stop(/* Only one device (#0) at a time for now (FIXME) */)
	{
		if (NULL != s_hMidiIn)
		{
			midiInStop(s_hMidiIn);
			midiInReset(s_hMidiIn);
			midiInClose(s_hMidiIn);

			midiInUnprepareHeader(s_hMidiIn, &s_header, sizeof(MIDIHDR));
		}		
		
		s_hMidiIn = NULL;
	}

	// Pull-style values
	float WinMidi_GetCutoff()    { return s_cutoff.Get();    }
	float WinMidi_GetResonance() { return s_resonance.Get(); }
	float WinMidi_GetFilterMix() { return s_filterMix.Get(); }
}
