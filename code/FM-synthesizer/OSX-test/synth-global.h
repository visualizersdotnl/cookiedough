
/*
	Syntherklaas FM -- Global includes, constants & utility functions.

	Dependencies on Bevacqua (as far as I know):
		- Std3DMath
		- Macros
*/

#ifndef _SFM_SYNTH_GLOBAL_H_
#define _SFM_SYNTH_GLOBAL_H_

// FIXME: only necessary when depending on the Kurt Bevacqua engine as our base
// #include "../main.h"

// FIXME: quick fix to compile on OSX
 #include "bevacqua-compat.h"

// Alias Bevacqua existing mechanisms (FIXME: adapt target platform's)
#define SFM_INLINE VIZ_INLINE
#define SFM_ASSERT VIZ_ASSERT

#include "synth-log.h"
#include "synth-error.h"
#include "synth-fast-math.h"
#include "sinus-LUT.h"

namespace SFM
{
	// Pretty standard sample rate, can always up it (for now the BASS hack in this codebase relies on it (FIXME)).
	const unsigned kSampleRate = 44100;
	const unsigned kMaxSamplesPerUpdate = kSampleRate/8;

	// Buffer size.
	const unsigned kRingBufferSize = kMaxSamplesPerUpdate;

	// Reasonable audible spectrum.
	const float kAudibleLowHz = 20.f;
	const float kAudibleHighHz = 22000.f;

	// LFO ranges.
	const float kMaxStdLFO = kAudibleLowHz;
	const float kMaxSonicLFO = kAudibleHighHz;	

	// Nyquist frequencies.
	const float kNyquist = kSampleRate/2.f;
	const float kAudibleNyquist = std::min<float>(kAudibleHighHz, kNyquist);

	// Max. number of voices (FIXME: more!)
	const unsigned kMaxVoices = 8;

	// Max. (or initial) voice amplitude
	const float kMaxVoiceAmplitude = 1.f - 1.f/kMaxVoices;

	// Number of discrete values that make up a period in the sinus LUT.
	const unsigned kSinLUTPeriod = kSinTabSize;
	const float kSinLUTCosOffs = kSinLUTPeriod/4;

	// Use to multiply modulation value to LUT pitch
	const float kLinToSinLUT = (1.f/k2PI)*kSinLUTPeriod;
	/*
		Utility functions.
	*/

	// Frequency to sinus LUT pitch
	SFM_INLINE float CalcSinLUTPitch(float frequency)
	{
		return (frequency*kSinLUTPeriod)/kSampleRate;
	}

	// Frequency to angular pitch
	SFM_INLINE float CalcAngularPitch(float frequency)
	{
		return (frequency/kSampleRate)*k2PI;
	}

	// This can be, for example, used to drive extra filtering
	SFM_INLINE float CalcNoiseRate(float frequency)
	{
		return (frequency*kSampleRate)/kAudibleNyquist;
	}

	// From and to dB
	inline float AmplitudeTodB(float amplitude) { return 20.f * log10f(amplitude); }
	inline float dBToAmplitude(float dB)        { return powf(10.f, dB/20.f);     }

	// For debug purposes
	SFM_INLINE bool IsNAN(float value)
	{
		return value != value;

		// FIXME: not supported on Unix?
//		return std::isnan(value);
	}
}

#endif // _SFM_SYNTH_GLOBAL_H_
