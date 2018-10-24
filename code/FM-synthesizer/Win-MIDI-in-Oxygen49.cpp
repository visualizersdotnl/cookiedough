
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	** This is not production quality code, it's just for my home rig. **

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

// #define DUMP_MIDI_EVENTS

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
	const unsigned kPotCutoff = 22;            // C11
	const unsigned kPotResonance = 23;         // C12
	const unsigned kPotFilterMix = 61;         // C10
	const unsigned kPotFilterEnvInfl = 24;     // C13
	const unsigned kPotMasterDrive = 26;       // C14
	const unsigned kPotMasterModLFOCurve = 95; // C17
	static MIDI_Smoothed s_cutoff, s_resonance, s_filterWetness, s_filterEnvInfl;
	static MIDI_Smoothed s_masterDrive;
	static MIDI_Smoothed s_masterModLFOCurve;

	// Wheel mapping
	const unsigned kMasterModIndex = 1;  // C32 (MOD wheel)
	static MIDI_Smoothed s_masterModIndex;

	// Fader mapping
	const unsigned kFaderA = 20; // C1
	const unsigned kFaderD = 21; // C2
	const unsigned kFaderS = 71; // C3
	const unsigned kFaderR = 72; // C4
	const unsigned kFaderMasterModRatioC = 70;     // C8
	const unsigned kFaderMasterModRatioM = 63;     // C9
	const unsigned kFaderMasterModLFOTilt = 25;    // C5
	const unsigned kFaderMasterModLFOFreq = 73;    // C6
	const unsigned kFaderMasterModBrightness = 74; // C7
	static MIDI_Smoothed s_A, s_D, s_S, s_R;
	static MIDI_Smoothed s_masterModRatioC, s_masterModRatioM;
	static MIDI_Smoothed s_masterModLFOTilt, s_masterModLFOFreq;
	static MIDI_Smoothed s_masterModBrightness;

	static Waveform s_waveform = kSine;

	// Percussion channel indices (used to select carrier waveform)
	const unsigned kPerc1 = 36;
	const unsigned kPerc2 = 38;
	const unsigned kPerc3 = 42;
	const unsigned kPerc4 = 46;
	const unsigned kPerc5 = 50;
	const unsigned kPerc6 = 45;
	const unsigned kPerc7 = 51;
	const unsigned kPerc8 = 49;

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

#ifdef DUMP_MIDI_EVENTS
				// Dumps incoming events, very useful
				static char buffer[128];
				sprintf(buffer, "MIDI input: Type %u Chan %u Idx %u Val %u Time %u", eventType, channel, controlIdx, controlVal, dwParam2);
				Log(buffer);
#endif

				if (CHANNEL_PERCUSSION == channel)
				{
					switch (controlIdx)
					{
					case kPerc1:
						s_waveform = kSine;
						break;

					case kPerc2:
						s_waveform = kSoftSaw;
						break;

					case kPerc3:
						s_waveform = kSoftSquare;
						break;

					case kPerc4:
						s_waveform = kDigiSaw;
						break;

					case kPerc5:
						s_waveform = kDigiSquare;
						break;

					case kPerc6:
						s_waveform = kTriangle;
						break;

					case kPerc7:
						s_waveform = kKick808;
						break;

					case kPerc8:
						s_waveform = kPinkNoise;
						break;

					default:
						s_waveform = kTriangle;
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
						case kPotCutoff:
							s_cutoff.Set(controlVal);
							break;

						case kPotResonance:
							s_resonance.Set(controlVal);
							break;

						case kPotFilterMix:
							s_filterWetness.Set(controlVal);
							break;

						case kPotMasterDrive:
							s_masterDrive.Set(controlVal);
							break;

						case kPotFilterEnvInfl:
							s_filterEnvInfl.Set(controlVal);
							break;

						case kMasterModIndex:
							s_masterModIndex.Set(controlVal);
							break;

						case kFaderA:
							s_A.Set(controlVal);
							break;

						case kFaderD:
							s_D.Set(controlVal);
							break;

						case kFaderS:
							s_S.Set(controlVal);
							break;

						case kFaderR:
							s_R.Set(controlVal);
							break;

						case kFaderMasterModRatioC:
							s_masterModRatioC.Set(controlVal);
							break;

						case kFaderMasterModRatioM:
							s_masterModRatioM.Set(controlVal);
							break;

						case kPotMasterModLFOCurve:
							s_masterModLFOCurve.Set(controlVal);
							break;

						case kFaderMasterModLFOTilt:
							s_masterModLFOTilt.Set(controlVal);
							break;

						case kFaderMasterModLFOFreq:
							s_masterModLFOFreq.Set(controlVal);
							break;

						case kFaderMasterModBrightness:
							s_masterModBrightness.Set(controlVal);
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
						const unsigned iVoice = TriggerNote(s_waveform, g_midiToFreqLUT[controlIdx], controlVal/127.f);
						s_voices[controlIdx] = iVoice;
						Log("NOTE_ON " + std::to_string(controlIdx));
						Log("iVoice: " + std::to_string(iVoice));
						break;
					}

				case NOTE_OFF:
					{
						Log("NOTE_OFF: " + std::to_string(controlIdx));
						const unsigned iVoice = s_voices[controlIdx];
						if (-1 != iVoice)
						{
							Log("RELEASED: " + std::to_string(iVoice));							
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
			Log("MIDI: implement MIM_LONGDATA!");
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
		Log("Using MIDI device " + std::string(devCaps.szPname));

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
	float WinMidi_GetFilterCutoff()    { return s_cutoff.Get();         }
	float WinMidi_GetFilterResonance() { return s_resonance.Get();      }
	float WinMidi_GetFilterWetness()   { return s_filterWetness.Get();  }
	float WinMidi_GetFilterEnvInfl()   { return s_filterEnvInfl.Get();  }

	// Master drive
	float WinMidi_GetMasterDrive()           { return s_masterDrive.Get(); }

	// Master modulation main
	float WinMidi_GetMasterModulationIndex()  { return s_masterModIndex.Get();  }
	float WinMidi_GetMasterModulationRatioC() { return s_masterModRatioC.Get(); }
	float WinMidi_GetMasterModulationRatioM() { return s_masterModRatioM.Get(); }

	// Master modulation LFO
	float WinMidi_GetMasterModLFOTilt()      { return s_masterModLFOTilt.Get();    }
	float WinMidi_GetMasterModLFOFrequency() { return s_masterModLFOFreq.Get();    }
	float WinMidi_GetMasterModLFOPower()     { return s_masterModLFOCurve.Get();   }
	float WinMidi_GetMasterModBrightness()   { return s_masterModBrightness.Get(); }

	// Master ADSR
	float WinMidi_GetMasterAttack()          { return s_A.Get(); }
	float WinMidi_GetMasterDecay()           { return s_D.Get(); }
	float WinMidi_GetMasterSustain()         { return s_S.Get(); }
	float WinMidi_GetMasterRelease()         { return s_R.Get(); }
}
