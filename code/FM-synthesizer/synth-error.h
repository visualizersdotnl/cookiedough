
/*
	Syntherklaas -- Error handling.
*/

#pragma once

namespace SFM
{
	enum ErrorLevel
	{
		kWarning,
		kInteraction,
		kFatal
	};

	SFM_INLINE void Error(ErrorLevel level, const char *message = nullptr)
	{
		switch (level)
		{
		case kWarning:
			break; // FIXME

		case kInteraction:
			break; // FIXME

		case kFatal:
			SetLastError((nullptr == message) ? "Syntherklaas FM: unspecified fatal error" : message);

		default:
			SFM_ASSERT(false);
		}
	}
};
