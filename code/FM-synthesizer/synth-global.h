
/*
	Syntherklaas FM -- Global includes, constants & utility functions.

	IMPORTANT: all of these are tweaked to be used with the BASS hack in it's host codebase.
*/

#ifndef _SFM_SYNTH_GLOBAL_H_
#define _SFM_SYNTH_GLOBAL_H_

// FIXME: only necessary when depending on the Kurt Bevacqua engine as our base
#include "../main.h"

// Rename existing mechanisms:
#define SFM_INLINE VIZ_INLINE
#define SFM_ASSERT VIZ_ASSERT

#include "sinus-LUT.h"
#include "synth-log.h"
#include "synth-error.h"

namespace SFM
{
	// Pretty standard sample rate, can always up it (for now the BASS hack in this codebase relies on it (FIXME)).
	const unsigned kSampleRate = 44100;
	const unsigned kMaxSamplesPerUpdate = 512;

	// Buffer size.
	const unsigned kRingBufferSize = kMaxSamplesPerUpdate;

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

	/*
		Utility functions.
	*/

	// Frequency to sinus LUT pitch
	SFM_INLINE float CalcSinPitch(float frequency)
	{
		return (frequency*kPeriodLength)/kSampleRate;
	}

	// Frequency to angular pitch
	SFM_INLINE float CalcAngularPitch(float frequency)
	{
		return (frequency*k2PI)/kSampleRate;
	}

	// This can be, for example, used to drive extra filtering
	SFM_INLINE float CalcNoiseRate(float frequency)
	{
		return (frequency*kSampleRate)/kAudibleNyquist;
	}

	// For debug purposes
	SFM_INLINE bool IsNAN(float value)
	{
		return value != value;
	}
}

#endif // _SFM_SYNTH_GLOBAL_H_
