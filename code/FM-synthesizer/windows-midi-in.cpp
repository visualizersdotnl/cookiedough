
/*
	Syntherklaas -- FM synthesizer prototype.
	Windows (Win32) MIDI input.

	FIXME:
		- Need mapping, error checking et cetera to spread the executable.
		- Curves are needed to make the knobs et cetera sound a bit more natural.
*/

#include "global.h"
#include "windows-midi-in.h"

// Bevacqua mainly relies on SDL + standard C(++), so:
#pragma comment(lib, "Winmm.lib")
#include <Windows.h>
#include <Mmsystem.h>

namespace SFM
{
	static HMIDIIN s_hMidiIn = NULL;

	static const size_t kMidiBufferLength = 256;
	static uint8_t s_buffer[kMidiBufferLength];

	static MIDIHDR s_header;

	// Test value to play with the M-AUDIO Oxygen 49
	static float s_test1 = 0.f;
	static float s_test2 = 0.f;
	static float s_test3 = 0.f;

	static void WinMidiProc(
		HMIDI hMidiIn,
		UINT wMsg,
		DWORD_PTR dwInstance,
		DWORD_PTR _dwParam1, 
		DWORD_PTR _dwParam2)
	{
		// Screw 32-bit warnings
		unsigned dwParam1 = (unsigned) _dwParam1; // Timestamp
		unsigned dwParam2 = (unsigned) _dwParam2; // Index and value

		// FIXME: debug only
		static TCHAR buffer[80];

		switch (wMsg)
		{
		case MIM_DATA:
		{
			// sprintf(buffer, "0x%08X 0x%02X 0x%02X 0x%02X\r\n", dwParam2, dwParam1 & 0x000000FF, (dwParam1>>8) & 0x000000FF, (dwParam1>>16) & 0x000000FF);
			// OutputDebugString(buffer);

			// FIXME-note: third parameter is index, fourth is value!
			auto controlIdx = (dwParam1>>8)&0xff;
			auto controlVal = (dwParam1>>16)&0xff;
			if (controlIdx == 0x16)
			{
				s_test1 = controlVal / 127.f;
				SFM_ASSERT(s_test1 >= 0.f && s_test1 <= 1.f);
			}
			else if (controlIdx == 0x17)
			{
				s_test2 = controlVal / 127.f;
				SFM_ASSERT(s_test2 >= 0.f && s_test2 <= 1.f);
			}
			else if (controlIdx == 0x1a)
			{
				s_test3 = controlVal / 127.f;
				SFM_ASSERT(s_test3 >= 0.f && s_test3 <= 1.f);
			}

			break;
		}

		case MIM_LONGDATA:
			// FIXME
			break;

		case MIM_OPEN:
		case MIM_CLOSE:
			// I think I'm handling this OK
			break; 

		case MIM_ERROR:
		case MIM_LONGERROR:
		case MIM_MOREDATA:
			// This means I'm at fault or falling behind
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

		const auto openRes = midiInOpen(&s_hMidiIn, devIdx, (DWORD_PTR) WinMidiProc, NULL, CALLBACK_FUNCTION);
		if (0 == openRes)
		{
			s_header.lpData = (LPSTR) s_buffer; // I'm not going to attempt to make this nonsene prettier.
			s_header.dwBufferLength = kMidiBufferLength;
			s_header.dwFlags = 0;

			const auto hdrPrepRes = midiInPrepareHeader(s_hMidiIn, &s_header, sizeof(MIDIHDR));
			if (0 == hdrPrepRes)
			{
				const auto queueRes = midiInAddBuffer(s_hMidiIn, &s_header, sizeof(MIDIHDR));
				if (0 == queueRes)
				{
					const auto startRes = midiInStart(s_hMidiIn);
					if (0 == startRes)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	void WinMidi_Stop(/* Only one device at a time for now (FIXME) */)
	{
		if (NULL != s_hMidiIn)
		{
			midiInClose(s_hMidiIn);
			midiInPrepareHeader(s_hMidiIn, &s_header, sizeof(MIDIHDR));
		}		
		
		s_hMidiIn = NULL;
	}

	// FIXME: prototype throw-away code
	float WinMidi_GetTestValue_1() { return s_test1; }
	float WinMidi_GetTestValue_2() { return s_test2; }
	float WinMidi_GetTestValue_3() { return s_test3; }
}
