
/*
	Syntherklaas FM -- Misc. small utility functions
*/

#pragma once

#include <cmath>
#include <limits>

#include "synth-math.h"

namespace SFM 
{
	// Frequency to pitch
	SFM_INLINE float CalculatePitch(float frequency)
	{
		return frequency/kSampleRate;
	}

	// Frequency to angular pitch
	SFM_INLINE float CalculateAngularPitch(float frequency)
	{
		return CalculatePitch(frequency)*k2PI;
	}

	// To and from dB
	SFM_INLINE float AmplitudeTodB(float amplitude) { return 20.f * log10f(amplitude); }
	SFM_INLINE float dBToAmplitude(float dB)        { return powf(10.f, dB/20.f);      }

	// Is frequency audible?
	inline bool InAudibleSpectrum(float frequency)
	{
		return frequency >= kAudibleLowHz && frequency <= kAudibleHighHz;
	}

	SFM_INLINE const bool IsPow2(unsigned value)
	{
		return value != 0 && !(value & (value - 1));
	}

	// Soft clamp
	SFM_INLINE float SoftClamp(float sample)
	{
		return fast_tanhf(sample);
	}

	// Hard clamp
	SFM_INLINE float Clamp(float sample)
	{
		if (sample > 1.f) 
			sample = 1.f;
		else if (sample < -1.f) 
			sample = -1.f;
		
		return sample;
	}

	// Good way to mix 2 samples
	SFM_INLINE float Mix(float sampleA, float sampleB)
	{
		return fast_tanhf(sampleA+sampleB);
	}

	/*
		Floating point error detection
		FIXME: does not compile on OSX?
	*/

	SFM_INLINE bool FloatCheck(float value)
	{
		if (value != 0.f && fabsf(value) < std::numeric_limits<float>::denorm_min())
			return false;
			 
		return !std::isnan(value);
	}

	/*
		Sample check function.

		Since dealing with floating point values creates, I cite, "many opportunities for difficulties", during development
		I want to check them often (in debug builds). The closer I am to the problem the quicker I can pin down what causes it.

		In release builds this vanishes.
	*/

	SFM_INLINE void SampleAssert(float sample)
	{
		SFM_ASSERT(sample >= -1.f && sample <= 1.f);
		SFM_ASSERT(true == FloatCheck(sample));
	}
}
