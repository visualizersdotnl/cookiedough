
/*
	Syntherklaas FM -- Misc. small utility functions
*/

#pragma once

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

	// For debug purposes
	SFM_INLINE bool IsNAN(float value)
	{
		return value != value;

		// FIXME: not supported on Unix?
		return std::isnan(value);
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

} 