
/*
	Syntherklaas FM: distortion for Wurlitzer mode.
*/

// #include "synth-global.h"
#include "synth-Wurlitzer-grit.h"

namespace SFM
{
	void WurlyGrit::SetCutoff(float cutoff)
	{
		SFM_ASSERT(cutoff >= 0.f && cutoff <= 1.f);
		const float lowC = 16.3f;
		const float cutoffHz = lowC + (kNyquist-lowC)*cutoff;

		// FIXME: analyze best filter
		m_filter.updateCoefficients(cutoff, 4.f /* FIXME: random */, SvfLinearTrapOptimised2::HIGH_SHELF_FILTER, kSampleRate);
	}

	SFM_INLINE float BitCrush(float sample, unsigned mask)
	{
		int integer = int(sample*65536.f);
		integer &= ~mask;
		return float(integer)/65536.f;
	}

	float WurlyGrit::Sample(float sample, float drive)
	{
		SFM_ASSERT(drive >= 0.f && drive <= 1.f);

		const float driven = sample;

		// FIXME: replace for S&H
		const float crushedHi = BitCrush(driven, 4096-1);
		const float crushedLo = BitCrush(driven, 8192-1);
		const float crushed = lerpf<float>(crushedHi, crushedLo, drive);

		const float filtered = (float) m_filter.tick(crushed);
		return filtered;
	}
}
