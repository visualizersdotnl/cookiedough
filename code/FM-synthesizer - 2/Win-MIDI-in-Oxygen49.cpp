
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	** This is not production quality code, it's just for my home setup. **

	- Keep it in P01 for this mapping to work (or adjust if it doesn't, I often screw around here).
	- Use the octave button to fiddle around the gamut.
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

	// FIXME: for SYSEX
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
	const unsigned kPotMasterDrive = 26;  // C14
	const unsigned kPotOpCoarse = 22;     // C10
	const unsigned kPotOpFine = 23;       // C11
	const unsigned kPotOpDetune = 61;     // C12
	const unsigned kPotOpAmplitude = 24;  // C13

	static float s_masterDrive = 0.f;
	static float s_opCoarse = 0.f, s_opFine = 0.f, s_opDetune = 0.f, s_opAmplitude = 0.f;

	// Wheel mapping
	const unsigned kModIndex = 1;  // C32 (MOD wheel)
	static float s_modIndex = 0.f;

	// Pitch bend (14-bit signed, wheel rests in the middle)
	static float s_pitchBend;

	static unsigned s_voices[127] = { -1 };

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
				const float fControlVal = controlVal/127.f;

#ifdef DUMP_MIDI_EVENTS
				// Dumps incoming events, very useful
				static char buffer[128];
				sprintf(buffer, "Oxy49 MIDI input: Type %u Chan %u Idx %u Val %u Time %u", eventType, channel, controlIdx, controlVal, dwParam2);
				Log(buffer);
#endif

				if (CHANNEL_PERCUSSION == channel)
				{
					return;
				}

				switch (eventType)
				{
				default:
					{
						switch (controlIdx)
						{
						/*  Master drive (voice volume) */

						case kPotMasterDrive:
							s_masterDrive = fControlVal;
							break;

						/* FM */

						case kModIndex:
							s_modIndex = fControlVal;
							break;

						case kPotOpCoarse:
							s_opCoarse = fControlVal;
							break;

						case kPotOpFine:
							s_opFine = fControlVal;
							break;

						case kPotOpDetune:
							s_opDetune = fControlVal;
							break;

						case kPotOpAmplitude:
							s_opAmplitude = fControlVal;
							break;
						}
					}

					break;
		
				case PITCH_BEND:
					{
						const unsigned bend = (controlVal<<7)|controlIdx;
						s_pitchBend = (float(bend)/8192.f) - 1.f;
						Log("Pitch bend: " + std::to_string(s_pitchBend));
						break;
					}

				case NOTE_ON:
					{
						if (-1 == s_voices[controlIdx])
							TriggerVoice(s_voices+controlIdx, Waveform::kSine, g_midiToFreqLUT[controlIdx], fControlVal);
						else
							// Only happens when CPU-bound past the limit
							Log("NOTE_ON could not be triggered due to latency.");

						break;
					}

				case NOTE_OFF:
					{
						const unsigned iVoice = s_voices[controlIdx];
						if (-1 != iVoice)
						{
							ReleaseVoice(iVoice, controlVal/127.f);
							s_voices[controlIdx] = -1;
						}

						break;
					}
				}
			}

			return;

		case MIM_LONGDATA:
			// FIXE: implement?
			Log("Oxy49 MIDI: implement MIM_LONGDATA!");
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

	SFM_INLINE unsigned WinMidi_GetNumDevices()
	{
		return midiInGetNumDevs();
	}

	bool WinMidi_Oxygen49_Start()
	{
		MIDIINCAPS devCaps;
		
		unsigned iOxygen49 = -1;

		const unsigned numDevices = WinMidi_GetNumDevices();
		for (unsigned iDev = 0; iDev < numDevices; ++iDev)
		{
			const MMRESULT result = midiInGetDevCaps(iDev, &devCaps, sizeof(MIDIINCAPS));
			SFM_ASSERT(MMSYSERR_NOERROR == result); 
			const std::string devName(devCaps.szPname);
 			
			if ("Oxygen 49" == devName)
			{
				iOxygen49 = iDev;
				break;
			}

			Log("Enumerated MIDI device: " + devName);
		}		

		if (-1 != iOxygen49)
		{
			Log("Detected correct MIDI device: " + std::string(devCaps.szPname));
			const unsigned devIdx = iOxygen49;

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
		}

		Error(kFatal, "Can't find the Oxygen 49 MIDI keyboard");

		return false;
	}

	void WinMidi_Oxygen49_Stop()
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

	// Master
	float WinMidi_GetMasterDrive() { return s_masterDrive; }

	// Operator control
	float WinMidi_GetOperatorCoarse()     { return s_opCoarse;    }
	float WinMidi_GetOperatorFine()       { return s_opFine;      }
	float WinMidi_GetOperatorDetune()     { return s_opDetune;    }
	float WinMidi_GetOperatorAmplitude()  { return s_opAmplitude; }

	// Modulation index
	float WinMidi_GetModulation() 
	{ 
		return s_modIndex; 
	}

	// Pitch bend
	float WinMidi_GetPitchBend() 
	{ 
		return s_pitchBend; 
	}

}
