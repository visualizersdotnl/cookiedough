	
/*
	Syntherklaas FM -- Windows (Win32) MIDI input explicitly designed for the M-AUDIO Oxygen 49.

	** This is not production quality code, it's just for my home setup. **

	- Keep it in P01 for this mapping to work (or adjust if it doesn't, I often screw around here).
	- Use the octave button to fiddle around the gamut.
*/

#include "synth-global.h"

#include "Win-MIDI-in-Oxygen49.h"
#include "Win-MIDI-in-BeatStep.h"

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
	const unsigned kPotMasterDrive = 26;       // C14
	const unsigned kPotCutoff = 22;            // C10
	const unsigned kPotResonance = 23;         // C12
	const unsigned kPotFilterContour = 61;     // C12
	const unsigned kPotFilterDrive =24;        // C13
	const unsigned kPotFeedback = 27;          // C15
	const unsigned kPotFeedbackWetness = 62;   // C16
	const unsigned kPot_MOOG_slaveFM = 95;     // C17

	static MIDI_Smoothed s_masterDrive;
	static MIDI_Smoothed s_cutoff;
	static float s_resonance = 0.f, s_filterContour = 0.f, s_filterDrive = 0.f;
	static float s_feedback = 0.f, s_feedbackWetness = 0.f;
	static float s_slaveFM = 0.f;

	// Wheel mapping
	const unsigned kModIndex = 1;  // C32 (MOD wheel)
	static float s_modIndex = 0.f;

	// Fader mapping
	const unsigned kFaderA = 20; // C1
	const unsigned kFaderD = 21; // C2
	const unsigned kFaderS = 71; // C3
	const unsigned kFaderR = 72; // C4
	const unsigned kFaderModRatio = 70;       // C8
	const unsigned kFaderFeedbackPitch = 63;  // C9
	const unsigned kFaderTremolo = 25;        // C5
	const unsigned kFaderModVibrato = 73;     // C6
	const unsigned kFaderModBrightness = 74;  // C7

	static float s_A = 0.f, s_D = 0.f, s_S = 0.f, s_R = 0.f;
	static float s_modRatio = 0.f;
	static float s_feedbackPitch = 0.f;
	static float s_modVibrato = 0.f;
	static float s_modBrightness = 0.f;
	static float s_tremolo = 0.f;

	// Button mapping
	const unsigned kButtonFilterSwitch = 116;        // C28
	const unsigned kButtonLoopWaves = 113;           // C27
	const unsigned kButtonAlgoSingle = 96;           // C18
	const unsigned kButtonAlgoDoubleCarriers = 97;   // C19
	const unsigned kButtonAlgoMiniMOOG = 98;         // C20
	const unsigned kButtonFlipFilterEnv = 117;       // C29

	static VoiceFilter s_curFilter = kButterworthFilter;
	static int s_loopWaves = 0;
	static Algorithm s_algorithm = kSingle;
	static bool s_flipFilterEnv = false;

	// Percussion channel indices
	const unsigned kPerc1 = 36;
	const unsigned kPerc2 = 38;
	const unsigned kPerc3 = 42;
	const unsigned kPerc4 = 46;
	const unsigned kPerc5 = 50;
	const unsigned kPerc6 = 45;
	const unsigned kPerc7 = 51;
	const unsigned kPerc8 = 49;

	static FormantShaper::Vowel s_formantVowel = FormantShaper::kNeutral;

	// Pitch bend (14-bit signed, wheel rests in the middle)
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
					case kPerc5:
						s_formantVowel = FormantShaper::kNeutral;
						break;

					case kPerc6:
						s_formantVowel = FormantShaper::kVowelA;
						break;

					case kPerc7:
						s_formantVowel = FormantShaper::kVowelE;
						break;

					case kPerc8:
						s_formantVowel = FormantShaper::kVowelI;
						break;

					case kPerc1:
						s_formantVowel = FormantShaper::kVowelO;
						break;

					case kPerc2:
						s_formantVowel = FormantShaper::kVowelU;
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
							s_masterDrive.Set(fControlVal);
							break;

						/* FM */

						case kModIndex:
							s_modIndex = fControlVal;
							break;

						case kFaderModRatio:
							s_modRatio = fControlVal;
							break;

						case kFaderModVibrato:
							s_modVibrato = fControlVal;
							break;

						case kFaderModBrightness:
							s_modBrightness = fControlVal;
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

						/* Feedback */

						case kFaderFeedbackPitch:
							s_feedbackPitch = fControlVal;
							break;

						case kPotFeedback:
							s_feedback = fControlVal;
							break;

						case kPotFeedbackWetness:
							s_feedbackWetness = fControlVal;
							break;

						/* Tremolo */

						case kFaderTremolo:
							s_tremolo = fControlVal;
							break;
						
						/* Algorithm select */

						case kButtonAlgoSingle:
							if (127 == controlVal) s_algorithm = kSingle;
							break;

						case kButtonAlgoDoubleCarriers:
							if (127 == controlVal) s_algorithm = kDoubleCarriers;
							break;

						case kButtonAlgoMiniMOOG:
							if (127 == controlVal) s_algorithm = kMiniMOOG;
							break;

						/* Algorithm #3 */

						case kPot_MOOG_slaveFM:
							s_slaveFM = fControlVal;
							break;

						/* Filter */

						case kButtonFlipFilterEnv:
							if (127 == controlVal) s_flipFilterEnv = !s_flipFilterEnv;
							break;

						case kButtonFilterSwitch:
							if (127 == controlVal) s_curFilter = VoiceFilter((s_curFilter+1) % kNumFilters);
							break;

						case kPotCutoff:
							s_cutoff.Set(fControlVal);
							break;

						case kPotResonance:
							s_resonance = fControlVal;
							break;

						case kPotFilterContour:
							s_filterContour = fControlVal;
							break;

						case kPotFilterDrive:
							s_filterDrive = fControlVal;
							break;

						/* One-shot */

						case kButtonLoopWaves:
							if (127 == controlVal) s_loopWaves ^= 1;
							break;
						
						default:
							break;
						}

						break;
					}
		
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
							TriggerVoice(s_voices+controlIdx, WinMidi_GetCarrierOscillator(), g_midiToFreqLUT[controlIdx], controlVal/127.f);
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

				return;
			}

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
	float WinMidi_GetMasterDrive() { return s_masterDrive.Get(); }

	// Filter
	VoiceFilter WinMidi_GetCurFilter()    { return s_curFilter;      }
	float WinMidi_GetFilterCutoff()       { return s_cutoff.Get();   }
	float WinMidi_GetFilterResonance()    { return s_resonance;      }
	float WinMidi_GetFilterContour()      { return s_filterContour;  }
	float WinMidi_GetFilterDrive()        { return s_filterDrive;    }
	bool  WinMidi_GetFilterFlipEnvelope() { return s_flipFilterEnv;  }

	// Feedback
	float WinMidi_GetFeedback()         { return s_feedback;         }
	float WinMidi_GetFeedbackWetness()  { return s_feedbackWetness;  }
	float WinMidi_GetFeedbackPitch()    { return s_feedbackPitch;    }

	// Pitch bend
	float WinMidi_GetPitchBend() { return s_pitchBend; }

	// Modulation main
	float WinMidi_GetModulationIndex()      { return s_modIndex;      }
	float WinMidi_GetModulationRatio()      { return s_modRatio;      }
	float WinMidi_GetModulationBrightness() { return s_modBrightness; }
	float WinMidi_GetModulationVibrato()    { return s_modVibrato;    }

	// Master ADSR
	float WinMidi_GetAttack()          { return s_A; }
	float WinMidi_GetDecay()           { return s_D; }
	float WinMidi_GetSustain()         { return s_S; }
	float WinMidi_GetRelease()         { return s_R; }

	// Tremolo
	float WinMidi_GetTremolo()
	{
		return s_tremolo;
	}

	// Loop wavetable samples on/off
	bool WinMidi_GetLoopWaves()
	{
		return 0 != s_loopWaves;
	}

	// Algorithm select
	Algorithm WinMidi_GetAlgorithm() { return s_algorithm; }

	// Algorithm #3
	float WinMidi_GetSlaveFM()
	{
		return s_slaveFM;
	}

	// Formant vowel
	FormantShaper::Vowel WinMidi_GetFormantVowel()
	{
		return s_formantVowel;
	}
}