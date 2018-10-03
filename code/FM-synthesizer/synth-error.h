
/*
	Syntherklaas -- FM synthesizer prototype.
	Error handling mechanism.
*/

#ifndef _SFM_ERROR_H_
#define _SFM_ERROR_H_

#include "global.h"

namespace SFM
{
	enum ErrorLevel
	{
		ERR_WARNING,
		ERR_INTERACTION,
		ERR_FATAL
	};

	void Error(ErrorLevel level)
	{
		switch (level)
		{
		case ERR_WARNING:
			break; // FIXME

		case ERR_INTERACTION:
			break; // FIXME

		case ERR_FATAL:
			SetLastError("Syntherklaas FM fatal error");
			break;

		default:
			SFM_ASSERT(false);
		}
	}
};

#endif // _SFM_SINUS_LUT_H_
