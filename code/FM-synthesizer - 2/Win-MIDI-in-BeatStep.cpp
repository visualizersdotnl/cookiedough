
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the Arturia BeatStep; an addition to the Oxygen 49.

	** This is not production quality code, it's just for my home setup. **
*/

#include "synth-global.h"

#include "Win-MIDI-in-BeatStep.h"

#include "synth-midi.h"
#include "FM_BISON.h"

#include "Win-MIDI-in-Oxygen49.h"

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
		Mapping for the BeatStep
	*/

	// Rotary mapping
	const unsigned kPotVibrato = 10;       // Set 1, R1
	const unsigned kPotOpVibrato = 74;     // Set 1, R2
	const unsigned kPotDelayWet = 114;     // Set 3, R9
	const unsigned kPotDelayRate = 18;     // Set 3, R10
	const unsigned kPotDelayWidth = 19;    // Set 3, R11
	const unsigned kPotDelayFeedback = 16; // Set 3, R12
	const unsigned kPotFilterA = 91;       // Set 4, R14
	const unsigned kPotFilterD = 79;       // Set 4, R15
	const unsigned kPotFilterS = 72;       // Set 4, R16
	const unsigned kPotFilterDrive = 75;   // Set 2, R8 
	const unsigned kPotNoteJitter = 17;    // Set 4, R13
	const unsigned kPotPitchA = 77;        // Set 2, R5
	const unsigned kPotPitchD = 93;        // Set 2, R6
	const unsigned kPotPitchL = 73;        // Set 2, R7
	const unsigned kPotVowelWet = 71;      // Set 1, R3
	const unsigned kPotVowelBlend = 76;    // Set 1, R4

	static float s_vibrato = 0.f;
	static float s_opVibrato[kNumOperators] = { 0.f };

	static float s_delayWet = 0.f;
	static float s_delayRate = 0.f;
	static float s_delayWidth = 0.f;
	static float s_delayFeedback = 0.f;

	static float s_filterA = 0.f;
	static float s_filterD = 0.f;
	static float s_filterS = 0.f;
	static float s_filterDrive = 0.f;

	static float s_noteJitter = 0.f;

	static float s_pitchA = 0.f;
	static float s_pitchD = 0.f;
	static float s_pitchL = 0.f;
	
	static float s_vowelWet = 0.f;
	static float s_vowelBlend = 0.f;

	// Button mapping
	const unsigned kButtonVowelA = 44;
	const unsigned kButtonVowelE = 45;
	const unsigned kButtonVowelI = 46;
	const unsigned kButtonVowelO = 47;
	const unsigned kButtonVowelU = 48;

	static VowelFilter::Vowel s_vowel = VowelFilter::kA;

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
				sprintf(buffer, "BeatStep MIDI input: Type %u Chan %u Idx %u Val %u Time %u", eventType, channel, controlIdx, controlVal, dwParam2);
				Log(buffer);
#endif

				static MIDIHDR s_header;

				if (0 == channel)
				{
					switch (controlIdx)
					{
					/* Vibrato */

					case kPotVibrato:
						s_vibrato = fControlVal;
						break;

					case kPotOpVibrato:
						s_opVibrato[g_currentOp] = fControlVal;
						break;

					/* Delay */

					case kPotDelayWet:
						s_delayWet = fControlVal;
						break;

					case kPotDelayRate:
						s_delayRate = fControlVal;
						break;

					case kPotDelayWidth:
						s_delayWidth = fControlVal;
						break;

					case kPotDelayFeedback:
						s_delayFeedback = fControlVal;
						break;

					/* Filter (ADS) */

					case kPotFilterA:
						s_filterA = fControlVal;
						break;

					case kPotFilterD:
						s_filterD = fControlVal;
						break;

					case kPotFilterS:
						s_filterS = fControlVal;
						break;


					/* Filter drive */

					case kPotFilterDrive:
						s_filterDrive = fControlVal;
						break;

					/* Note jitter */

					case kPotNoteJitter:
						s_noteJitter = fControlVal;
						break;

					/* Pitch env. (AD) */

					case kPotPitchA:
						s_pitchA = fControlVal;
						break;

					case kPotPitchD:
						s_pitchD = fControlVal;
						break;

					case kPotPitchL:
						s_pitchL = fControlVal;
						break;

					/* Vowel filter */

					case kPotVowelWet:
						s_vowelWet = fControlVal;
						break;

					case kPotVowelBlend:
						s_vowelBlend = fControlVal;
						break;

					case kButtonVowelA:
						s_vowel = VowelFilter::kA;
						break;

					case kButtonVowelE:
						s_vowel = VowelFilter::kE;
						break;

					case kButtonVowelI:
						s_vowel = VowelFilter::kI;
						break;

					case kButtonVowelO:
						s_vowel = VowelFilter::kO;
						break;

					case kButtonVowelU:
						s_vowel = VowelFilter::kU;
						break;
					}
				}

				return;
			}

		case MIM_LONGDATA:
			// FIXE: implement?
			Log("BeatStep MIDI: implement MIM_LONGDATA!");
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

	bool WinMidi_BeatStep_Start()
	{
		MIDIINCAPS devCaps;
		
		unsigned iBeatStep = -1;

		const unsigned numDevices = WinMidi_GetNumDevices();
		for (unsigned iDev = 0; iDev < numDevices; ++iDev)
		{
			const MMRESULT result = midiInGetDevCaps(iDev, &devCaps, sizeof(MIDIINCAPS));
			SFM_ASSERT(MMSYSERR_NOERROR == result); 
			const std::string devName(devCaps.szPname);
 			
			if ("Arturia BeatStep" == devName)
			{
				iBeatStep = iDev;
				break;
			}

			Log("Enumerated MIDI device: " + devName);
		}		

		if (-1 != iBeatStep)
		{
			Log("Detected correct MIDI device: " + std::string(devCaps.szPname));
			const unsigned devIdx = iBeatStep;

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
						const auto startRes = midiInStart(s_hMidiIn);
						if (MMSYSERR_NOERROR == startRes)
						{
							return true;
						}
					}
				}
			}
		}

		Error(kFatal, "Can't find/open the BeatStep MIDI keyboard");

		return false;
	}

	void WinMidi_BeatStep_Stop()
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

	// Vibrato
	float WinMidi_GetVibrato()         { return s_vibrato; }
	float WinMidi_GetOperatorVibrato() { return s_opVibrato[WinMidi_GetOperator()]; }
	
	// Delay
	float WinMidi_GetDelayWet()      { return s_delayWet;      }
	float WinMidi_GetDelayRate()     { return s_delayRate;     }
	float WinMidi_GetDelayWidth()    { return s_delayWidth;    }
	float WinMidi_GetDelayFeedback() { return s_delayFeedback; }

	// Filter
	float WinMidi_GetFilterA()     { return s_filterA; }
	float WinMidi_GetFilterD()     { return s_filterD; }
	float WinMidi_GetFilterS()     { return s_filterS; }
	float WinMidi_GetFilterDrive() { return s_filterDrive; }

	// Note jitter
	float WinMidi_GetNoteJitter() { return s_noteJitter; } 

	// Pitch env.
	float WinMidi_GetPitchA() { return s_pitchA; }
	float WinMidi_GetPitchD() { return s_pitchD; }
	float WinMidi_GetPitchL() { return s_pitchL; }

	// Vowel filter
	float WinMidi_GetVowelWet()   { return s_vowelWet;   }
	float WinMidi_GetVowelBlend() { return s_vowelBlend; } 

	VowelFilter::Vowel WinMidi_GetVowel()
	{
		return s_vowel;
	}
}
