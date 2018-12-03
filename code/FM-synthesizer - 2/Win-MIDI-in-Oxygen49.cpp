
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	** This is not production quality code, it's just for my home setup. **

	- Keep it in P01 for this mapping to work (or adjust if it doesn't, I often screw around here).
	- Use the octave button to fiddle around the gamut.

	Operator editing is a little ham-fisted, but these are the rules:
	- Press percussion pad that corresponds with operator.
	- Set all parameters to what they should be.
	- Hold red button (C30) to set.

	FIXME: notes may be outdated
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
	const unsigned kPotCutoff = 27;       // C15
	const unsigned kPotResonance = 62;    // C16
	const unsigned kPotFilterWet = 95;    // C17

	// Fader mapping
	const unsigned kFaderA = 20;             // C1
	const unsigned kFaderD = 21;             // C2
	const unsigned kFaderS = 71;             // C3
	const unsigned kFaderR = 72;             // C4
	const unsigned kFaderOpTremolo = 25;     // C5
	const unsigned kFaderTremolo = 73;       // C6
	const unsigned kFaderOpEnvA = 74;        // C7
	const unsigned kFaderOpEnvD = 70;        // C8
	const unsigned kFaderOpFeedbackAmt = 63; // C9

	static float s_masterDrive = 0.f;
	static float s_tremolo = 0.f;

	static float s_opEnvA[kNumOperators] = { 0.f };
	static float s_opEnvD[kNumOperators] = { 0.f };

	static float s_cutoff = 0.f;
	static float s_resonance = 0.f;
	static float s_filterWet = 0.f;

	// Keep a copy per operator to make the interface a tad more intuitive
	static bool  s_opFixed[kNumOperators]        = { false };
	static float s_opFeedbackAmt[kNumOperators]  = { 0.f };
	static float s_opCoarse[kNumOperators]       = { 0.f };
	static float s_opFine[kNumOperators]         = { 0.f }; 
	static float s_opDetune[kNumOperators]       = { 0.f }; 
	static float s_opAmplitude[kNumOperators]    = { 0.f };
	static float s_opTremolo[kNumOperators]      = { 0.f };

	static float s_A = 0.f, s_D = 0.f, s_S = 0.f, s_R = 0.f;

	// Button mapping
	const unsigned kButtonOpRecv = 118;      // C30
	const unsigned kButtonFilterType1 = 103; // C25
	const unsigned kButtonFilterType2 = 104; // C26
	const unsigned kButtonFilterInv = 102;   // C24
	const unsigned kButtonOpFixed = 116;     // C28

	// Wheel mapping
	const unsigned kModIndex = 1;  // C32 (MOD wheel)
	static float s_modIndex = 0.f;

	// Pitch bend (special message, 14-bit signed, wheel rests in the middle)
	static float s_pitchBend;

	// Percussion channel indices
	const unsigned kPerc1 = 36;
	const unsigned kPerc2 = 38;
	const unsigned kPerc3 = 42;
	const unsigned kPerc4 = 46;
	const unsigned kPerc5 = 50;
	const unsigned kPerc6 = 45;
	const unsigned kPerc7 = 51;
	const unsigned kPerc8 = 49;

	static int s_filterType = 0;
	static bool s_filterInv = false;

	// Current (receiving) operator
	/* static */ unsigned g_currentOp = 0;
	static bool s_opRecv = false;

	// LFO shape
	static Waveform s_LFOShape = kSine;

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
					switch (controlIdx)
					{
					/* Current operator */

					case kPerc1:
						g_currentOp = 0;
						break;

					case kPerc2:
						g_currentOp = 1;
						break;

					case kPerc3:
						g_currentOp = 2;
						break;

					case kPerc4:
						g_currentOp = 3;
						break;

					case kPerc5:
						g_currentOp = 4;
						break;

					case kPerc6:
						g_currentOp = 5;
						break;

					/* LFO shape */

					case kPerc7:
						s_LFOShape = kSine;
						break;

					case kPerc8:
						s_LFOShape = kDigiTriangle;
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
						/*  Master drive (voice volume) */

						case kPotMasterDrive:
							s_masterDrive = fControlVal;
							break;

						/* Master vibrato */

						case kFaderTremolo:
							s_tremolo = fControlVal;
							break;

						/* Filter */

						case kPotCutoff:
							s_cutoff = fControlVal;
							break;

						case kPotResonance:
							s_resonance = fControlVal;
							break;

						case kPotFilterWet:
							s_filterWet = fControlVal;
							break;

						case kButtonFilterType1:
							if (127 == controlVal) s_filterType = 0;
							break;

						case kButtonFilterType2:
							if (127 == controlVal) s_filterType = 1;
							break;

						case kButtonFilterInv:
							if (127 == controlVal) s_filterInv = !s_filterInv;
							break;

						/* Operator */

						case kButtonOpRecv:
							s_opRecv = (127 == controlVal);
							break;

						case kButtonOpFixed:
							if (127 == controlVal) s_opFixed[g_currentOp] ^= 1; // Toggle

							// Quite handy to keep an eye on this whilst we're in hardware dev. mode
							Log("Operator " + std::to_string(g_currentOp+1) + " set to ratio mode: " + std::to_string(s_opFixed[g_currentOp]));

							break;

						case kFaderOpFeedbackAmt:
							s_opFeedbackAmt[g_currentOp] = fControlVal;
							break;

						case kModIndex:
							s_modIndex = fControlVal;
							break;

						case kPotOpCoarse:
							s_opCoarse[g_currentOp] = fControlVal;
							break;

						case kPotOpFine:
							s_opFine[g_currentOp] = fControlVal;
							break;

						case kPotOpDetune:
							s_opDetune[g_currentOp] = fControlVal;
							break;

						case kPotOpAmplitude:
							s_opAmplitude[g_currentOp] = fControlVal;
							break;

						case kFaderOpTremolo:
							s_opTremolo[g_currentOp] = fControlVal;
							break;

						/* Operator envelope */

						case kFaderOpEnvA:
							s_opEnvA[g_currentOp] = fControlVal;
							break;

						case kFaderOpEnvD:
							s_opEnvD[g_currentOp] = fControlVal;
							break;

						/* ADSR */

						case kFaderA:
							s_A = fControlVal;
							break;

						case kFaderD:
							s_D= fControlVal;
							break;

						case kFaderS:
							s_S= fControlVal;
							break;

						case kFaderR:
							s_R = fControlVal;
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

	// Master & Tremolo
	float WinMidi_GetMasterDrive() { return s_masterDrive; }
	float WinMidi_GetTremolo()     { return s_tremolo;   }

	// Operator control
	bool  WinMidi_GetOperatorFixed()          { return s_opFixed[g_currentOp];       }
	float WinMidi_GetOperatorCoarse()         { return s_opCoarse[g_currentOp];      }
	float WinMidi_GetOperatorFinetune()       { return s_opFine[g_currentOp];        }
	float WinMidi_GetOperatorDetune()         { return s_opDetune[g_currentOp];      }
	float WinMidi_GetOperatorAmplitude()      { return s_opAmplitude[g_currentOp];   }
	float WinMidi_GetOperatorTremolo()        { return s_opTremolo[g_currentOp];     }
	float WinMidi_GetOperatorFeedbackAmount() { return s_opFeedbackAmt[g_currentOp]; }

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

	// Current operator (if receiving)
	unsigned WinMidi_GetOperator()
	{
		return (true == s_opRecv) ? g_currentOp : -1;
	}

	// Master ADSR
	float WinMidi_GetAttack()          { return s_A; }
	float WinMidi_GetDecay()           { return s_D; }
	float WinMidi_GetSustain()         { return s_S; }
	float WinMidi_GetRelease()         { return s_R; }

	// Modulation envelope (attack & decay)
	float WinMidi_GetOperatorEnvA() { return s_opEnvA[g_currentOp]; }
	float WinMidi_GetOperatorEnvD() { return s_opEnvD[g_currentOp]; }

	// Operaetor LFO shape
	Waveform WinMidi_GetLFOShape() { return s_LFOShape; }

	// Filter
	int   WinMidi_GetFilterType() { return s_filterType; }
	float WinMidi_GetCutoff()     { return s_cutoff;     } 
	float WinMidi_GetResonance()  { return s_resonance;  }
	float WinMidi_GetFilterWet()  { return s_filterWet;  }
	bool  WinMidi_GetFilterInv()  { return s_filterInv;  }
}
