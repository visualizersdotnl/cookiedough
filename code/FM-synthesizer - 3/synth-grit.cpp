
/*
	Syntherklaas FM: Grit distortion.
*/

#include "synth-grit.h"

namespace SFM
{
	void Grit::SetCutoff(float cutoff)
	{
		SFM_ASSERT(cutoff >= 16.f && cutoff <= kNyquist);
		m_filter.updateCoefficients(cutoff, 0.5f, SvfLinearTrapOptimised2::LOW_SHELF_FILTER, kSampleRate);
	}


	float Grit::Sample(float sample, float drive)
	{
		SFM_ASSERT(drive >= 0.f && drive <= 1.f);

		// TO DO:
		// - Work it out until it sounds good, then move it to per-voice
		// - Idea: use velocity as a pre-filter parameter
		// ...

		// Float crush
		float diameter = powf(2.f, 1.1f + 2.f*drive);
		float radius = diameter*0.5f;
		float inverse = 1.f/radius;
		float crushed = inverse * int(sample*radius);

		// Filter
		const float filtered = (float) m_filter.tick(crushed);
		return filtered;
	}
}
