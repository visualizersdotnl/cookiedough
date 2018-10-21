
/*
	Syntherklaas FM -- Misc. small utility functions
*/

#pragma once

#include <cmath>
#include <limits>

namespace SFM 
{
	// Frequency to sinus LUT pitch
	SFM_INLINE float CalculateOscPitch(float frequency)
	{
		return (frequency*kOscPeriod)/kSampleRate;
	}

	// Frequency to angular pitch
	SFM_INLINE float CalculateAngularPitch(float frequency)
	{
		return (frequency/kSampleRate)*k2PI;
	}

	// This can be, for example, used to drive extra filtering
	SFM_INLINE float CalculateNoiseRate(float frequency)
	{
		return (frequency*kSampleRate)/kAudibleNyquist;
	}

	// From and to dB
	inline float AmplitudeTodB(float amplitude) { return 20.f * log10f(amplitude); }
	inline float dBToAmplitude(float dB)        { return powf(10.f, dB/20.f);      }

	// For assertions, mostly
	inline bool InAudibleSpectrum(float frequency)
	{
		return frequency >= kAudibleLowHz && frequency <= kAudibleHighHz;
	}

	// Unsigned integer is-power-of-2 check
	SFM_INLINE const bool IsPow2(unsigned value)
	{
		return value != 0 && !(value & (value - 1));
	}

	// Hard clamp
	SFM_INLINE float Clamp(float sample)
	{
		return clampf(-1.f, 1.f, sample);
	}

	/*
		Floating point f*ckup detection
		FIXME: didn't seem to compile on OSX
	*/

	SFM_INLINE bool FloatCheck(float value)
	{
		if (value != 0.f && fabsf(value) < std::numeric_limits<float>::denorm_min())
			return false;

		return !std::isnan(value);
	}
}
