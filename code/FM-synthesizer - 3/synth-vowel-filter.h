
/*
	Syntherklaas FM -- Simple vowel (formant shaper) filter.

	Code adapted from a post by alex@smartelectronix.com (public domain, http://www.musicdsp.org)
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	class VowelFilter
	{
	public:
		enum Vowel
		{
			kA,
			kE,
			kI,
			kO,
			kU,
			kNumVowels
		};

	private:
		alignas(16) double m_ring[2][10];

		float Calculate(float sample, Vowel vowelA, unsigned iBuf);

	public:
		VowelFilter()
		{
			Reset();
		}

		void Reset()
		{
			memset(m_ring[0], 0, 10*sizeof(double));
			memset(m_ring[1], 0, 10*sizeof(double));
		}

		// Interpolates towards next neighbour
		float Apply(float sample, Vowel vowelA, float delta);
	};
}
