
/*
	Syntherklaas FM -- Basic formant shaper (filter).

	Code adapted from a post by alex@smartelectronix.com (public domain, http://www.musicdsp.org)
*/

#pragma once

namespace SFM
{
	enum FormantVowel
	{
		kVowelA,
		kVowelE,
		kVowelI,
		kVowelO,
		kVowelU,
		kNumVowels
	};

	class FormantShaper
	{
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

		float Apply(float sample, FormantVowel vowel);
	};
}
