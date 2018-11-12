
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the Arturia BeatStep; an addition to the Oxygen 49.

	** This is not production quality code, it's just for my home setup. **
*/

#include "synth-global.h"
#include "Win-MIDI-in-BeatStep.h"
#include "synth-midi.h"
#include "FM_BISON.h"
#include "synth-voice.h"

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
	const unsigned kPot_MOOG_SlavesDetune = 7; // Level/Rate (big rotary)
	const unsigned kPotNoisyness = 17;         // Set 4, R13
	const unsigned kPotPulseWidth = 91;        // Set 4, R14
	const unsigned kPotFilterA = 10;           // Set 1, R1
	const unsigned kPotFilterD = 74;           // Set 1, R2
	const unsigned kPotFilterS = 71;           // Set 1, R3
	const unsigned kPotDoubleDetune = 72;      // Set 4, R16
	const unsigned kPotDoubleVolume = 79;      // Set 4, R15
	const unsigned kPot_MOOG_slavesLP = 73;    // Set 2, R7
	const unsigned kPot_MOOG_CarrierVol1 = 77; // Set 2, R5
	const unsigned kPot_MOOG_CarrierVol2 = 93; // Set 2, R6
	const unsigned kPot_MOOG_CarrierVol3 = 75; // Set 2, R8
	const unsigned kPotNintendize = 16;        // Set 3, R12
	const unsigned kPotFormant = 19;           // Set 3, R11
	const unsigned kPotFormantStep = 18;       // Set 3, R10
	const unsigned kPotBassBoost = 114;        // Set 3, R9
	const unsigned kPotNoteDrift = 76;         // Set 1, R4

	static float s_noisyness = 0.f;
	static float s_pulseWidth = 0.f;
	static float s_filterA = 0.f, s_filterD = 0.f, s_filterS = 0.f;
	static float s_doubleDetune = 0.f;
	static float s_doubleVolume = 0.f;
	static float s_slavesDetune = 0.f;
	static MIDI_Smoothed s_slavesLP;
	static float s_mCarrierVol1 = 0.f, s_mCarrierVol2 = 0.f, s_mCarrierVol3 = 0.f;
	static float s_Nintendize = 0.f;
	static MIDI_Smoothed s_formant, s_formantStep(0.33f);
	static float s_bassBoost = 0.f;
	static float s_noteDrift = 0.f;

	// Button mapping

	// Procedural
	const unsigned kButtonOscSine = 44;            // Button 1
	const unsigned kButtonOscTriangle = 45;        // Button 2
	const unsigned kButtonOscAnalogueSaw = 46;     // Button 3
	const unsigned kButtonOscAnalogueSquare = 47;  // Button 4
	const unsigned kButtonOscPulse = 48;           // Button 5
	const unsigned kButtonOscNoise = 49;           // Button 6
	const unsigned kButtonOsc2_Sin = 50;           // Button 7
	const unsigned kButtonOsc2_Pulse = 42;         // Button 15
	const unsigned kButtonOsc3_Triangle = 51;      // Button 8
	const unsigned kButtonOsc3_Square = 43;        // Button 16
	const unsigned kButton_MOOG_HardSync_On = 41;  // Button 14
	const unsigned kButton_MOOG_HardSync_Off = 40; // Button 13

	static bool s_hardSync = false;

	// Wavetable
	const unsigned kButtonOscPiano = 36;    // Button 9
	const unsigned kButtonOsc808Kick = 37;; // Button 10
	const unsigned kButtonOsc808Snare = 38; // Button 11
	const unsigned kButtonOscGuitar = 39;   // Button 12

	// Oscillators (defaults by hardware layout)
	static Waveform s_waveform = kSine;
	static Waveform s_waveformOsc2 = kSine;
	static Waveform s_waveformOsc3 = kPolyTriangle;
	
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
					/* Algorithm #3 */

					case kButton_MOOG_HardSync_On:
						s_hardSync = true;
						break;

					case kButton_MOOG_HardSync_Off:
						s_hardSync = false;
						break;

					case kPot_MOOG_SlavesDetune:
						s_slavesDetune = fControlVal;
						break;

					case kPot_MOOG_slavesLP:
						s_slavesLP.Set(fControlVal);
						break;

					case kPot_MOOG_CarrierVol1:
						s_mCarrierVol1 = fControlVal;
						break;

					case kPot_MOOG_CarrierVol2:
						s_mCarrierVol2 = fControlVal;
						break;

					case kPot_MOOG_CarrierVol3:
						s_mCarrierVol3 = fControlVal;
						break;

					case kButtonOsc2_Sin:
						s_waveformOsc2 = kSine;
						break;

					case kButtonOsc2_Pulse:
						s_waveformOsc2 = kPolyPulse;
						break;

					case kButtonOsc3_Triangle:
						s_waveformOsc3 = kPolyTriangle;
						break;

					case kButtonOsc3_Square:
						s_waveformOsc3 = kPolySquare;
						break;

					/* Algorithm #2 */

					case kPotDoubleDetune:
						s_doubleDetune = fControlVal;
						break;

					case kPotDoubleVolume:
						s_doubleVolume = fControlVal;
						break;

					/* Noisyness */

					case kPotNoisyness:
						s_noisyness = fControlVal;
						break;

					/* Formant */
					
					case kPotFormant:
						s_formant.Set(fControlVal);
						break;

					case kPotFormantStep:
						s_formantStep.Set(fControlVal);
						break;

					/* Nintendize */

					case kPotNintendize:
						s_Nintendize = fControlVal;
						break;

					/* Bass boost */
					
					case kPotBassBoost:
						s_bassBoost = fControlVal;
						break;

					/* Note (VCO) drift */

					case kPotNoteDrift:
						s_noteDrift= fControlVal;
						break;

					/* Filter */
					
					case kPotFilterA:
						s_filterA = fControlVal;
						break;

					case kPotFilterD:
						s_filterD = fControlVal;
						break;

					case kPotFilterS:
						s_filterS = fControlVal;
						break;

					/* Oscillators */

					case kPotPulseWidth:
						s_pulseWidth = fControlVal;
						break;

					case kButtonOscSine:
						s_waveform = kSine;
						break;

					case kButtonOscTriangle:
						s_waveform = kPolyTriangle;
						break;

					case kButtonOscAnalogueSaw:
						s_waveform = kPolySaw;
						break;

					case kButtonOscAnalogueSquare:
						s_waveform = kPolySquare;
						break;

					case kButtonOscPulse:
						s_waveform = kPolyPulse;
						break;

					case kButtonOscNoise:
						s_waveform = kWhiteNoise;
						break;

					case kButtonOscPiano:
						s_waveform = kElectricPiano;
						break;

					case kButtonOsc808Kick:
						s_waveform = kKick808;
						break;

					case kButtonOsc808Snare:
						s_waveform = kSnare808;
						break;

					case kButtonOscGuitar:
						s_waveform = kGuitar;
						break;

					default:
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

		Error(kFatal, "Can't find the BeatStep MIDI keyboard");

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

	// Noisyness
	float WinMidi_GetNoisyness() { return s_noisyness; }

	// Bass boost
	float WinMidi_GetBassBoost() { return s_bassBoost; }

	// Nintendize
	float WinMidi_GetNintendize() { return s_Nintendize; }

	// Filter
	float WinMidi_GetFilterAttack()   { return s_filterA; }
	float WinMidi_GetFilterDecay()    { return s_filterD; }
	float WinMidi_GetFilterSustain()  { return s_filterS; }

	// Oscillators
	Waveform WinMidi_GetCarrierOscillator() { return s_waveform;   }
	float    WinMidi_GetPulseWidth()        { return s_pulseWidth; }

	// Algorithm #2
	float WinMidi_GetDoubleDetune() { return s_doubleDetune; }
	float WinMidi_GetDoubleVolume() { return s_doubleVolume; }

	// Algorithm #3
	float WinMidi_GetCarrierVolume1()        { return s_mCarrierVol1;   }
	float WinMidi_GetCarrierVolume2()        { return s_mCarrierVol2;   }
	float WinMidi_GetCarrierVolume3()        { return s_mCarrierVol3;   }
	float WinMidi_GetSlavesLowpass()         { return s_slavesLP.Get(); }
	float WinMidi_GetSlavesDetune()          { return s_slavesDetune;   }
	bool  WinMidi_GetHardSync()              { return s_hardSync;       }
	Waveform WinMidi_GetCarrierOscillator2() { return s_waveformOsc2;   }
	Waveform WinMidi_GetCarrierOscillator3() { return s_waveformOsc3;   }

	// Formant
	float WinMidi_GetFormant()     { return s_formant.Get();     }
	float WinMidi_GetFormantStep() { return s_formantStep.Get(); }

	// Note (VCO) drift
	float WinMidi_GetNoteDrift()
	{
		return s_noteDrift;
	}
}
