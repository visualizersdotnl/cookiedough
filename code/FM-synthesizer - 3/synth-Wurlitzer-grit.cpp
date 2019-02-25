
/*
	Syntherklaas FM: distortion for Wurlitzer mode.
*/

// #include "synth-global.h"
#include "synth-Wurlitzer-grit.h"

namespace SFM
{
	void WurlyGrit::SetCutoff(float cutoffHz)
	{
//		SFM_ASSERT(cutoff >= 0.f && cutoff <= 1.f);
//		const float lowC = 16.3f;
//		const float cutoffHz = lowC + (kNyquist-lowC)*cutoff;
		SFM_ASSERT(cutoffHz >= 16.f && cutoffHz <= kNyquist);

		// FIXME: pick best filter
		m_filter.updateCoefficients(cutoffHz, 33.f /* FIXME: random? */, SvfLinearTrapOptimised2::HIGH_SHELF_FILTER, kSampleRate);
	}

	float WurlyGrit::Sample(float sample)
	{
		m_phase += m_frequency;
		if (m_phase >= 1.f)
		{
			m_phase -= 1.f;
			m_hold = floorf(sample/m_step + .5f)*m_step;
		}

		return (float) m_filter.tick(m_hold);
	}
}
