
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	** This is not production quality code, it's just for my home rig. **

	- Keep it in P01 for this mapping to work (!).
	- Use the octave button!
*/

#include "synth-global.h"
#include "Win-MIDI-in-Oxygen49.h"
#include "synth-midi.h"
#include "FM_BISON.h"

// Bevacqua mainly relies on SDL + std. C(++), so:
#pragma comment(lib, "Winmm.lib")
#include <Windows.h>
#include <Mmsystem.h>

#define DUMP_MIDI_EVENTS

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

	/*
		Mapping for the Oxy-49's Patch #01
	*/

	// Rotary mapping
	const unsigned kPotCutoff = 22;      // C11
	const unsigned kPotResonance = 23;   // C12
	const unsigned kPotFilterMix = 61;   // C10
	const unsigned kPotMasterDrive = 26; // C14
	MIDI_Smoothed s_cutoff, s_resonance, s_filterWetness;
	MIDI_Smoothed s_masterDrive;

	// Wheel mapping
	const unsigned kMasterModIndex = 1;  // C32 (MOD wheel)
	MIDI_Smoothed s_masterModIndex;

	// Fader mapping
	const unsigned kFaderA = 20; // C1
	const unsigned kFaderD = 21; // C2
	const unsigned kFaderS = 71; // C3
	const unsigned kFaderR = 72; // C4
	MIDI_Smoothed s_A, s_D, s_S, s_R;

	// Mapping: 49 keys
	const unsigned kUpperKey = 36;
	const unsigned kLowerKey = 84;

	SFM_INLINE bool IsKey(unsigned index) 
	{ 
		return index >= kUpperKey && index <= kLowerKey; 
	}

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

				if (CHANNEL_PERCUSSION == channel)
				{
					// This is where I can use those nice buttons
					return;
				}

#ifdef DUMP_MIDI_EVENTS
				// Dumps incoming events, very useful
				static char buffer[128];
				sprintf(buffer, "MIDI input: Type %u Chan %u Idx %u Val %u Time %u", eventType, channel, controlIdx, controlVal, dwParam2);
				Log(buffer);
#endif

				switch (eventType)
				{
				default: // FIXME: == 13 (?)
					{
						switch (controlIdx)
						{
						case kPotCutoff:
							s_cutoff.Set(controlVal, dwParam2);
							break;

						case kPotResonance:
							s_resonance.Set(controlVal, dwParam2);
							break;

						case kPotFilterMix:
							s_filterWetness.Set(controlVal, dwParam2);
							break;

						case kPotMasterDrive:
							s_masterDrive.Set(controlVal, dwParam2);
							break;

						case kMasterModIndex:
							s_masterModIndex.Set(controlVal, dwParam2);
							break;

						case kFaderA:
							s_A.Set(controlVal, dwParam2);
							break;

						case kFaderD:
							s_D.Set(controlVal, dwParam2);
							break;

						case kFaderS:
							s_S.Set(controlVal, dwParam2);
							break;

						case kFaderR:
							s_R.Set(controlVal, dwParam2);
							break;

						default:
							break;
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
						const unsigned iVoice = TriggerNote(g_midiToFreqLUT[controlIdx], controlVal/127.f);
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
			s_header.lpData = (LPSTR) s_buffer;
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

	/*
		Pull-style controls
	*/

	// Filter
	float WinMidi_GetFilterCutoff()    { return s_cutoff.Get(); }
	float WinMidi_GetFilterResonance() { return s_resonance.Get(); }
	float WinMidi_GetFilterWetness()   { return s_filterWetness.Get(); }

	// Master
	float WinMidi_GetMasterDrive()           { return s_masterDrive.Get(); }
	float WinMidi_GetMasterModulationIndex() { return s_masterModIndex.Get(); }
	float WinMidi_GetMasterAttack()          { return s_A.Get(); }
	float WinMidi_GetMasterDecay()           { return s_D.Get(); }
	float WinMidi_GetMasterSustain()         { return s_S.Get(); }
	float WinMidi_GetMasterRelease()         { return s_R.Get(); }
}
