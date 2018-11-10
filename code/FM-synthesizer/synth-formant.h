
/*
	Syntherklaas FM -- Basic formant shaper (filter).

	Code adapted from a post by alex@smartelectronix.com (public domain, http://www.musicdsp.org)
*/

#pragma once

namespace SFM
{
	class FormantShaper
	{
	public:
		// A-U are positive to use as index
		enum Vowel
		{
			kNeutral   = -1,
			kVowelA    =  0,
			kVowelE    =  1,
			kVowelI    =  2,
			kVowelO    =  3,
			kVowelU    =  4,
			kNumVowels =  5
		};

	private:
		alignas(16) double m_rings[2][10];

		float Calculate(float sample, int vowel, unsigned iBuf);

	public:
		FormantShaper()
		{
			Reset();
		}

		void Reset()
		{
			memset(m_rings[0], 0, 10*sizeof(double));
			memset(m_rings[1], 0, 10*sizeof(double));
		}

		// Step should be [0..1] and blends between vowel and it's next neighbour
		float Apply(float sample, int vowel, float step = 0.f);
	};
}
