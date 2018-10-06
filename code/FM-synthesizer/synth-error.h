
/*
	Syntherklaas -- Error handling.
*/

#ifndef _SFM_SYNTH_ERROR_H_
#define _SFM_SYNTH_ERROR_H_

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
			SetLastError((nullptr == message) ? "Syntherklaas FM unspecified fatal error" : message);
			break;

		default:
			SFM_ASSERT(false);
		}
	}
};

#endif // _SFM_SYNTH_ERROR_H_

