
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	** This is not production quality code, it's just for my home setup. **
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

	// Pad channel indices
	const unsigned kPad1 = 36;
	const unsigned kPad2 = 38;
	const unsigned kPad3 = 42;
	const unsigned kPad4 = 46;
	const unsigned kPad5 = 50;
	const unsigned kPad6 = 45;
	const unsigned kPad7 = 51;
	const unsigned kPad8 = 49;

	// Current operator
	unsigned g_currentOp = 0;

	// Button indices
	const unsigned kButtonFixedRatio = 104;

	static bool s_opFixedRatio[kNumOperators] = { false };

	// Fader indices
	const unsigned kFaderAttack       = 20;
	const unsigned kFaderDecay        = 21;
	const unsigned kFaderSustain      = 71;
	const unsigned kFaderRelease      = 72;
	const unsigned kFaderAttackLevel  = 25;
	const unsigned kFaderUnused2      = 73;
	const unsigned kFaderCoarse       = 74;
	const unsigned kFaderFine         = 70;
	const unsigned kFaderDetune       = 63;

	// Operator envelope
	static float s_opAttack[kNumOperators]      = { 0.f };
	static float s_opDecay[kNumOperators]       = { 0.f };
	static float s_opSustain[kNumOperators]     = { 0.f };
	static float s_opRelease[kNumOperators]     = { 0.f };
	static float s_opAttackLevel[kNumOperators] = { 0.f };

	// Operator frequency
	static float s_opCoarse[kNumOperators] = { 0.f };
	static float s_opFine[kNumOperators]   = { 0.f }; 
	static float s_opDetune[kNumOperators] = { 0.f };

	// Rotary indices
	const unsigned kPotOutput = 22;
	const unsigned kPotVelSens = 23;
	const unsigned kPotFeedback = 61;

	// Various operator parameters
	static float s_opOutput[kNumOperators]   = { 0.f };
	static float s_opVelSens[kNumOperators]  = { 0.f };
	static float s_opFeedback[kNumOperators] = { 0.f };

	/*
		Mapping for the Oxy-49's Patch #01
	*/

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
				const float fControlVal = controlVal/127.f;

#ifdef DUMP_MIDI_EVENTS
				// Dumps incoming events, very useful
				static char buffer[128];
				sprintf(buffer, "Oxy49 MIDI input: Type %u Chan %u Idx %u Val %u Time %u", eventType, channel, controlIdx, controlVal, dwParam2);
				Log(buffer);
#endif

				if (CHANNEL_PERCUSSION == channel)
				{
					switch (controlIdx)
					{
					case kPad1:
						g_currentOp = 0;
						break;

					case kPad2:
						g_currentOp = 1;
						break;

					case kPad3:
						g_currentOp = 2;
						break;

					case kPad4:
						g_currentOp = 3;
						break;

					case kPad5:
						g_currentOp = 4;
						break;

					case kPad6:
						g_currentOp = 5;

					default:
						break;
					}

					return;
				}

				switch (eventType)
				{
				default:
					{
						switch (controlIdx)
						{
						/* Feedback */

						case kPotFeedback:
							s_opFeedback[g_currentOp] = fControlVal;
							break;

						/* Output level + Velocity sensitivity */

						case kPotOutput:
							s_opOutput[g_currentOp] = fControlVal;
							break;

						case kPotVelSens:
							s_opVelSens[g_currentOp] = fControlVal;
							break;

						/* Operator envelope */

						case kFaderAttack:
							s_opAttack[g_currentOp] = fControlVal;
							break;

						case kFaderDecay:
							s_opDecay[g_currentOp] = fControlVal;
							break;

						case kFaderSustain:
							s_opSustain[g_currentOp] = fControlVal;
							break;

						case kFaderRelease:
							s_opRelease[g_currentOp] = fControlVal;
							break;

						case kFaderAttackLevel:
							s_opAttackLevel[g_currentOp] = fControlVal;
							break;

						/* Operator frequency */

						case kButtonFixedRatio:
							if (127 == controlVal) s_opFixedRatio[g_currentOp] ^= 1;
							break;

						case kFaderCoarse:
							s_opCoarse[g_currentOp] = fControlVal;
							break;

						case kFaderFine:
							s_opFine[g_currentOp] = fControlVal;
							break;

						case kFaderDetune:
							s_opDetune[g_currentOp] = fControlVal;
							break;

						default:
							break;
						}
					}

					break;
		
				case PITCH_BEND:
					{
						const unsigned bend = (controlVal<<7)|controlIdx;
//						Log("Pitch bend: " + std::to_string(s_pitchBend));
						break;
					}

				case NOTE_ON:
					{
						if (-1 == s_voices[controlIdx])
							TriggerVoice(s_voices+controlIdx, Waveform::kSine, controlIdx, fControlVal);
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

		Error(kFatal, "Can't find/open the Oxygen 49 MIDI keyboard");

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

	// Operator envelope
	float WinMidi_GetOpAttack(unsigned iOp)       { return s_opAttack[iOp];  }
	float WinMidi_GetOpDecay(unsigned iOp)        { return s_opDecay[iOp];   }
	float WinMidi_GetOpSustain(unsigned iOp)      { return s_opSustain[iOp]; }
	float WinMidi_GetOpRelease(unsigned iOp)      { return s_opRelease[iOp]; }
	float WinMidi_GetOpAttackLevel(unsigned iOp)  { return s_opAttackLevel[iOp]; }

	// Operator frequency
	bool  WinMidi_GetOpFixed(unsigned iOp)  { return s_opFixedRatio[iOp]; }
	float WinMidi_GetOpCoarse(unsigned iOp) { return s_opCoarse[iOp];     }
	float WinMidi_GetOpFine(unsigned iOp)   { return s_opFine[iOp];       }
	float WinMidi_GetOpDetune(unsigned iOp) { return s_opDetune[iOp];     }

	// Output level
	float WinMidi_GetOpOutput(unsigned iOp) {
		return s_opOutput[iOp]; }

	// Velocity sensitivity
	float WinMidi_GetOpVelSens(unsigned iOp) {
		return s_opVelSens[iOp]; }

	// Feedback
	float WinMidi_GetFeedback(unsigned iOp) {
		return s_opFeedback[iOp]; }
}
