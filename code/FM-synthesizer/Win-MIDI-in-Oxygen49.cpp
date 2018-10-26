
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
	const unsigned kPotCutoff = 22;            // C11
	const unsigned kPotResonance = 23;         // C12
	const unsigned kPotFilterMix = 61;         // C10
	const unsigned kPotFilterEnvInfl = 24;     // C13
	const unsigned kPotMasterDrive = 26;       // C14
	const unsigned kPotMasterModLFOCurve = 95; // C17
	const unsigned kPotFeedback = 27;          // C15
	const unsigned kPotFeedbackWetness = 62;   // C16
	static MIDI_Smoothed s_cutoff, s_resonance, s_filterWetness, s_filterEnvInfl;
	static MIDI_Smoothed s_masterDrive;
	static MIDI_Smoothed s_masterModLFOCurve;
	static MIDI_Smoothed s_feedback, s_feedbackWetness;

	// Wheel mapping
	const unsigned kMasterModIndex = 1;  // C32 (MOD wheel)
	static MIDI_Smoothed s_masterModIndex;

	// Fader mapping
	const unsigned kFaderA = 20; // C1
	const unsigned kFaderD = 21; // C2
	const unsigned kFaderS = 71; // C3
	const unsigned kFaderR = 72; // C4
	const unsigned kFaderMasterModRatio = 70;      // C8
	const unsigned kFaderPhaser = 63;              // C9
	const unsigned kFaderMasterModLFOShape = 25;   // C5
	const unsigned kFaderMasterModLFOFreq = 73;    // C6
	const unsigned kFaderMasterModBrightness = 74; // C7
	static float s_A = 0.f, s_D = 0.f, s_S = 0.f, s_R = 0.f;
	static float s_masterModRatio = 0.f;
	static float s_phaser = 0.f;
	static float s_masterModLFOShape = 0.f, s_masterModLFOFreq = 0.f;
	static float s_masterModBrightness = 0.f;

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

	// Pitch bend
	static MIDI_Smoothed s_pitchBendS(0.5f, 16383); // 14-bit signed, wheel rests in the middle
	static float s_pitchBend;

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
				sprintf(buffer, "Oxy49 MIDI input: Type %u Chan %u Idx %u Val %u Time %u", eventType, channel, controlIdx, controlVal, dwParam2);
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
						s_waveform = kSnare808;
						break;

					default:
						s_waveform = kSine;
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

						case kPotFeedback:
							s_feedback.Set(controlVal);
							break;

						case kPotFeedbackWetness:
							s_feedbackWetness.Set(controlVal);
							break;

						case kMasterModIndex:
							s_masterModIndex.Set(controlVal);
							break;

						case kFaderA:
							s_A = controlVal/127.f;
							break;

						case kFaderD:
							s_D= controlVal/127.f;
							break;

						case kFaderS:
							s_S= controlVal/127.f;
							break;

						case kFaderR:
							s_R = controlVal/127.f;
							break;

						case kFaderMasterModRatio:
							s_masterModRatio = controlVal/127.f;
							break;

						case kFaderPhaser:
							s_phaser = controlVal/127.f;
							break;

						case kPotMasterModLFOCurve:
							s_masterModLFOCurve.Set(controlVal);
							break;

						case kFaderMasterModLFOShape:
							s_masterModLFOShape = controlVal/127.f;
							break;

						case kFaderMasterModLFOFreq:
							s_masterModLFOFreq = controlVal/127.f;
							break;

						case kFaderMasterModBrightness:
							s_masterModBrightness = controlVal/127.f;
							break;

						default:
							break;
						}

						break;
					}
		
				case PITCH_BEND:
					{
						const unsigned bend = (controlVal<<7)|controlIdx;
						s_pitchBendS.Set(bend);
						s_pitchBend = float(bend)/8192.f - 1.f;
						break;
					}

				case NOTE_ON:
					{
						const unsigned iVoice = TriggerVoice(s_waveform, g_midiToFreqLUT[controlIdx], controlVal/127.f);
						
						if (iVoice != -1)
							s_voices[controlIdx] = iVoice;
						
						break;
					}

				case NOTE_OFF:
					{
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
		Log("Using MIDI device: " + std::string(devCaps.szPname));

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

	// Feedback
	float WinMidi_GetFeedback()         { return s_feedback.Get();        }
	float WinMidi_GetFeedbackWetness()  { return s_feedbackWetness.Get(); }
	float WinMidi_GetFeedbackPhaser()   { return s_phaser; }

	// Master drive & pitch bend
	float WinMidi_GetMasterDrive() { return s_masterDrive.Get(); }

	float WinMidi_GetPitchBend()   
	{ 
		return s_pitchBendS.Get(); 
	}

	float WinMidi_GetPitchRaw()   
	{ 
		return s_pitchBend;
	}

	// Master modulation main
	float WinMidi_GetMasterModulationIndex()  { return s_masterModIndex.Get();  }
	float WinMidi_GetMasterModulationRatio()  { return s_masterModRatio;        }

	// Master modulation LFO
	float WinMidi_GetMasterModLFOShape()     { return s_masterModLFOShape;       }
	float WinMidi_GetMasterModLFOFrequency() { return s_masterModLFOFreq;        }
	float WinMidi_GetMasterModLFOPower()     { return s_masterModLFOCurve.Get(); }
	float WinMidi_GetMasterModBrightness()   { return s_masterModBrightness;     }

	// Master ADSR
	float WinMidi_GetMasterAttack()          { return s_A; }
	float WinMidi_GetMasterDecay()           { return s_D; }
	float WinMidi_GetMasterSustain()         { return s_S; }
	float WinMidi_GetMasterRelease()         { return s_R; }
}
