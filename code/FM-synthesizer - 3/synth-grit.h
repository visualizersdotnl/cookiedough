
/*
	Syntherklaas FM: Grit distortion.

	This approach was devised to add dirt to the top end, taking velocity into account.
	It's use case is the Wurlitzer approximation.
*/

#pragma once

#include "synth-global.h"
#include "3rdparty/SvfLinearTrapOptimised2.hpp"

namespace SFM
{
	class Grit
	{
	public:
		Grit() { Reset(); }

		void Reset()
		{
			m_filter.resetState();
			SetCutoff(kNyquist/2.f); // Attempt to just cut off n oise

			m_phase = 0.f;
			m_hold = 0.f;
		}
	
		void SetCutoff(float cutoff);

	public:
		float Sample(float sample, float drive);

	private:
		 SvfLinearTrapOptimised2 m_filter;

		 float m_phase;
		 float m_hold;
	};

	
}