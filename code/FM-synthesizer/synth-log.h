
/*
	Syntherklaas FM -- Logging (use sparingly, mainly for debug).

	FIXME: formatting!
*/

#pragma once

#if defined(WIN32) || defined(_WIN32)
	#include <Windows.h>
	#define SFM_logprint OutputDebugStringA
#else
	#define SFM_logprint printf
#endif

namespace SFM
{
	static char s_logBuffer[256]; // FIXME: overflow hazard

	static void Log(const char *message)
	{
		SFM_ASSERT(nullptr != message);
		sprintf(s_logBuffer, "SFM log: %s\n", message);
		SFM_logprint(s_logBuffer);
	}
}
