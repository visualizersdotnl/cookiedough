
/*
	Syntherklaas FM -- Formant shaper (filter).

	This is a rather cheap way of doing this, but the alternative (bandpassing 3 times per vowel)
	is probably not worth the cost; I most of all want this to be an effect.
*/

#include "synth-global.h"
#include "synth-formant.h"
#include "synth-util.h" 

const double kVowelCoeffs[SFM::FormantShaper::kNumVowels][11]= 
{
	// A
	{ 
		8.11044e-06,
		8.943665402, -36.83889529, 92.01697887, -154.337906, 181.6233289,
		-151.8651235, 89.09614114, -35.10298511, 8.388101016,  -0.923313471
	},

	// E
	{
		4.36215e-06,
		8.90438318, -36.55179099, 91.05750846, -152.422234, 179.1170248,
		-149.6496211, 87.78352223, -34.60687431, 8.282228154, -0.914150747
	},

	// I
	{ 
		3.33819e-06,
		8.893102966, -36.49532826, 90.96543286, -152.4545478, 179.4835618,
		-150.315433, 88.43409371, -34.98612086, 8.407803364, -0.932568035
	},

	// O
	{
		1.13572e-06,
		8.994734087, -37.2084849, 93.22900521, -156.6929844, 184.596544,
		-154.3755513, 90.49663749, -35.58964535, 8.478996281, -0.929252233
	},

	// U
	{
		4.09431e-07,
		8.997322763, -37.20218544, 93.11385476, -156.2530937, 183.7080141,
		-153.2631681, 89.59539726, -35.12454591, 8.338655623, -0.910251753
	}
};

namespace SFM
{
	float FormantShaper::Calculate(float sample, int vowel, unsigned iBuf)
	{
		SFM_ASSERT(vowel < kNumVowels);
		SFM_ASSERT(iBuf < 2);

		if (kNeutral == vowel)
		{
			return sample;
		}

		double *buffer = m_rings[iBuf];

		double result;
		result  = kVowelCoeffs[vowel][0]  * sample;
		result += kVowelCoeffs[vowel][1]  * buffer[0];
		result += kVowelCoeffs[vowel][2]  * buffer[1];
		result += kVowelCoeffs[vowel][3]  * buffer[2];
		result += kVowelCoeffs[vowel][4]  * buffer[3];
		result += kVowelCoeffs[vowel][5]  * buffer[4];
		result += kVowelCoeffs[vowel][6]  * buffer[5];
		result += kVowelCoeffs[vowel][7]  * buffer[6];
		result += kVowelCoeffs[vowel][8]  * buffer[7];
		result += kVowelCoeffs[vowel][9]  * buffer[8];
		result += kVowelCoeffs[vowel][10] * buffer[9];

		buffer[9] = buffer[8];
		buffer[8] = buffer[7];
		buffer[7] = buffer[6];
		buffer[6] = buffer[5];
		buffer[5] = buffer[4];
		buffer[4] = buffer[3];
		buffer[3] = buffer[2];
		buffer[2] = buffer[1];
		buffer[1] = buffer[0];
		buffer[0] = result;

		
		// Takes the edge off suprisingly well without screwing up the effect
		result = fast_tanh(result);

		return float(result);
	}

	float FormantShaper::Apply(float sample, int vowel, float step /* = 0.f */)
	{
		const unsigned vowelA = vowel;
		const int vowelB = (vowelA+1) % kNumVowels;

		SFM_ASSERT(step >= 0.f && step <= 1.f);
		const float result = lerpf<float>(Calculate(sample, vowelA, 0), Calculate(sample, vowelB, 1), step*step);

		SampleAssert(result);

		return result;
	}
}
