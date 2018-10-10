
/*
	Syntherklaas FM -- Logging (use sparingly, mainly for debug).
*/

#ifndef _SFM_SYNTH_LOG_H_
#define _SFM_SYNTH_LOG_H_

#if defined(WIN32) || defined(_WIN32)
	#include <Windows.h>
	#define SFM_logprint OutputDebugStringA
#else
	#define SFM_logprint printf
#endif

namespace SFM
{
	// Always dirty but it'll do for now (FIXME / FIXME #2: edited for OSX test hack)
	static char s_logBuffer[256]; 

	// FIXME: enable formatting
	static void Log(const char *message)
	{
		SFM_ASSERT(nullptr != message);

		printf("FM. BISON: %s\n", message);

//		sprintf(s_logBuffer, "Syntherklaas FM: %s\n", message);
//		SFM_logprint(s_logBuffer);
	}
}

#endif // _SFM_SYNTH_LOG_H_
