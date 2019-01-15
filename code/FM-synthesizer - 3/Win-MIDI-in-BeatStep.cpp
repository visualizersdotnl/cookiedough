
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

	// Current operator (Win-MIDI-in-Oxygen49.cpp)
	extern unsigned g_currentOp;

	// Rotary indices
	const unsigned kPotLiveliness = 114;
	const unsigned kPotLFOSpeed = 7; // The big knob
	const unsigned kPotFilterCutoff = 17;
	const unsigned kPotFilterResonance = 91;
	const unsigned kPotPickupDist = 77;
	const unsigned kPotPickAsym = 93;

	// Would've liked these on the Oxygen 49, but I'm out of controls :-)
	const unsigned kPotOpEnvRateMul = 72;
	const unsigned kPotOpEnvRateScale = 75;

	// Button indices
	const unsigned kButtonLFOSync = 36;
	const unsigned kButtonChorus = 44;

	// Liveliness
	static float s_liveliness = 0.f;

	// LFO speed
	static float s_LFOSpeed = 0.f;

	// LFO key sync.
	static bool s_LFOSync = false;

	// Chorus switch
	static bool s_chorus = false;

	// Operator env. rate
	static float s_opEnvRateMul[kNumOperators];
	static float s_opEnvRateScale[kNumOperators] = { 0.f };

	// Filter parameters
	static float s_cutoff = 1.f;
	static float s_resonance = 0.f;

	// Pickup parameters
	static float s_pickupDist = 1.f;
	static float s_pickupAsym = 1.f;

	/*
		Mapping for the BeatStep
	*/

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
					// Global controls (misc.)

					case kPotLiveliness:
						s_liveliness = fControlVal;
						break;

					case kPotLFOSpeed:
						s_LFOSpeed = fControlVal;
						break;

					case kButtonLFOSync:
						if (127 == controlVal) s_LFOSync ^= 1;
						break;

					case kButtonChorus:
						if (127 == controlVal) s_chorus ^= 1;
						break;

					// Pickup parameters

					case kPotPickupDist:
						s_pickupDist = fControlVal;
						break;

					case kPotPickAsym:
						s_pickupAsym = fControlVal;
						break;

					// Filter parameters

					case kPotFilterCutoff:
						s_cutoff = fControlVal;
						break;

					case kPotFilterResonance:
						s_resonance = fControlVal;
						break;

					// Operator envelope rate multiplier
					case kPotOpEnvRateMul:
						s_opEnvRateMul[g_currentOp] = 0.100f + (9.9f*fControlVal); // [0.1..10.0]
						break;

					// Operator envelope rate scaling
					case kPotOpEnvRateScale:
						s_opEnvRateScale[g_currentOp] = fControlVal;
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
							// Exception: initialize all to 1.0
							for (int iOp = 0; iOp < kNumOperators; ++iOp)
								s_opEnvRateMul[iOp] = 1.f;

							return true;
						}
					}
				}
			}
		}

		Error(kFatal, "Can't find/open the BeatStep MIDI controller");

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

	// Liveliness
	float WinMidi_GetLiveliness() {
		return s_liveliness; }

	// LFO speed
	float WinMidi_GetLFOSpeed() {
		return s_LFOSpeed;
	}

	// LFO key sync.
	bool WinMidi_GetLFOSync() {
		return s_LFOSync;
	}

	// Chorus switch
	bool WinMidi_ChorusEnabled() {
		return s_chorus;
	}

	// Operator env. rate 
	float WinMidi_GetOpEnvRateMul(unsigned iOp)    { return s_opEnvRateMul[iOp];   }
	float WinMidi_GetOpEnvRateScale(unsigned iOp)  { return s_opEnvRateScale[iOp]; }

	// Filter parameters
	float WinMidi_GetFilterCutoff()     { return s_cutoff;    }
	float WinMidi_GetFilterResonance()  { return s_resonance; }

	// Pickup parameters
	float WinMidi_GetPickupDist() { return s_pickupDist; }
	float WinMidi_GetPickupAsym() { return s_pickupAsym; }
}
