
/*
	Syntherklaas FM
	(C) syntherklaas.org, a subsidiary of visualizers.nl
	-- Global constants & main/top include.
*/

#ifndef _SMF_GLOBAL_H_
#define _SMF_GLOBAL_H_

// FIXME: only necessary when depending on the Kurt Bevacqua engine as our base
#include "../main.h"

// Rename:
#define SFM_INLINE VIZ_INLINE
#define SFM_ASSERT VIZ_ASSERT

// ...
#include "sinus-LUT.h"

namespace SFM
{
	// Pretty standard sample rate, can always up it (for now the BASS hack in this codebase relies on it (FIXME)).
	const unsigned kSampleRate = 44100;
	const unsigned kMaxSamplesPerUpdate = kSampleRate/4;

	// Reasonable audible spectrum.
	const float kAudibleLowHZ = 20.f;
	const float kAudibleHighHZ = 22000.f;

	// Nyquist frequencies.
	const float kNyquist = kSampleRate/2.f;
	const float kAudibleNyquist = std::min<float>(kAudibleHighHZ, kNyquist);

	// Max. number of voices (FIXME: more!)
	const unsigned kMaxVoices = 6;

	// Number of discrete values that make up a period in the sinus LUT.
	const unsigned kPeriodLength = kSinTabSize;

	// Useful when using actual radians with sinus LUT;
	const float kTabToRad = (1.f/k2PI)*kPeriodLength;
};

#endif // _SMF_GLOBAL_H_

