
/*
	Syntherklaas FM -- Debug logging.
*/

#include "synth-global.h"
// #include "synth-log.h"

namespace SFM
{

#if SFM_NO_LOGGING // Set in synth-global.h

	void Log(const std::string &mesage) {}

#else

#if defined(WIN32) || defined(_WIN32)

	#include <Windows.h>
	#define SFM_logprint OutputDebugStringA

	static char s_logBuffer[512];

	void Log(const std::string &message)
	{
		sprintf_s(s_logBuffer, 512, "SFM log: %s\n", message.c_str());
		OutputDebugStringA(s_logBuffer);
	}

#else

	static void Log(const std::string &message)
	{
		printf("SFM log: %s\n", message.c_str());
	}

#endif // PLATFORM

#endif // SFM_NO_LOGGING

}
