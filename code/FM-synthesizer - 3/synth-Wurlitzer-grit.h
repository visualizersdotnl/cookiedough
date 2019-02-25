
/*
	Syntherklaas FM: distortion for Wurlitzer mode.
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
		WurlyGrit() { Reset(); }

		void Reset(float cutoff = 1.f)
		{
			m_filter.resetState();
			SetCutoff(cutoff);
		}
	
		void SetCutoff(float cutoff);

	public:
		float Sample(float sample, float drive);

	private:
		 SvfLinearTrapOptimised2 m_filter;
	};

	
}