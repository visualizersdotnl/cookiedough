
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
		enum Vowel
		{
			kNeutral   = -1,
			kVowelA    =  0,
			kVowelE    =  1,
			kVowelI    =  2,
			kVowelO    =  3,
			kVowelU    =  4,
			kNumVowels
		};

	private:
		double m_memory[10];

	public:
		FormantShaper()
		{
			Reset();
		}

		void Reset()
		{
			memset(m_memory, 0, 10*sizeof(double));
		}

		float Apply(float sample, Vowel vowel);
	};
}
