
/*
	Syntherklaas FM: distortion for Wurlitzer mode.

	IDEA: use Karplus-Strong pluck to steer grit influence!
*/

#pragma once

#include "synth-global.h"
#include "3rdparty/SvfLinearTrapOptimised2.hpp"

namespace SFM
{
	// Cheap pickup distortion
	const float kDefPickupDist = 1.f;
	const float kDefPickupAsym = 0.3f;

	SFM_INLINE float fPickup(float sample)
	{
		SampleAssert(sample);
		const float z = sample+kDefPickupAsym;
		return 1.f/(kDefPickupDist + z*z*z);
	}
	
	// Grit (crush) filter
	class WurlyGrit
	{
	public:
		WurlyGrit() { Reset(0.5f, 0.5f, kNyquist); }

		void Reset(float horizontal, float vertical, float cutoffHz)
		{
			m_filter.resetState();
			SetCutoff(cutoffHz);

			// FIXME
			horizontal = 0.1f + 0.8f*horizontal;
			vertical   = 0.1f + 0.8f*vertical;

			m_step = 1.f / powf(2.f, (1.f-vertical)*15.f + 1.f);
			m_frequency = powf(1.f-horizontal, 2.f);

			m_phase = 0.f;
			m_hold = 0.f;
		}
	
		void SetCutoff(float cutoffHz);

	public:
		float Sample(float sample);

	private:
		 SvfLinearTrapOptimised2 m_filter;
		
		float m_step;
		float m_frequency;

		float m_phase;
		float m_hold;
	};

	
}