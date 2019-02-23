
/*
	Syntherklaas FM: Grit distortion.

	This approach was devised to add dirt to the top end, taking velocity into account.
	It's use case is the Wurlitzer approximation.
*/

#pragma once

#include "synth-global.h"
#include "3rdparty/SvfLinearTrapOptimised2.hpp"
// #include "synth-oscillator.h"

namespace SFM
{
	class Grit
	{
	public:
		void Reset(float frequency)
		{
			m_filter.resetState();
			SetCutoff(std::max<float>(16.f, frequency*0.1f));
		}
	
	private:
		void SetCutoff(float cutoff)
		{
			m_filter.updateCoefficients(cutoff, 1.5f, SvfLinearTrapOptimised2::LOW_PASS_FILTER, kSampleRate);
		}

	public:
		float Apply(float sample, float factor);

	private:
		 SvfLinearTrapOptimised2 m_filter;
	};

	
}