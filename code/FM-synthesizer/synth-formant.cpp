
/*
	Syntherklaas FM -- Formant shaper (filter).

	FIXME:
		- This is a rather cheap and lo-fi way of doing this, but the alternative (bandpassing 3 times per vowel)
		  is probably not worth the cost
		- Use ring buffer
*/

#include "synth-global.h"
#include "synth-formant.h"

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
	float FormantShaper::Apply(float sample, int vowel)
	{
		SFM_ASSERT(vowel < kNumVowels);			

		if (kNeutral == vowel)
			return sample;

		double result;
			
		result  = kVowelCoeffs[vowel][0]  * sample;
		result += kVowelCoeffs[vowel][1]  * m_memory[0];
		result += kVowelCoeffs[vowel][2]  * m_memory[1];
		result += kVowelCoeffs[vowel][3]  * m_memory[2];
		result += kVowelCoeffs[vowel][4]  * m_memory[3];
		result += kVowelCoeffs[vowel][5]  * m_memory[4];
		result += kVowelCoeffs[vowel][6]  * m_memory[5];
		result += kVowelCoeffs[vowel][7]  * m_memory[6];
		result += kVowelCoeffs[vowel][8]  * m_memory[7];
		result += kVowelCoeffs[vowel][9]  * m_memory[8];
		result += kVowelCoeffs[vowel][10] * m_memory[9];

		m_memory[9] = m_memory[8];
		m_memory[8] = m_memory[7];
		m_memory[7] = m_memory[6];
		m_memory[6] = m_memory[5];
		m_memory[5] = m_memory[4];
		m_memory[4] = m_memory[3];
		m_memory[3] = m_memory[2];
		m_memory[2] = m_memory[1];
		m_memory[1] = m_memory[0];

		m_memory[0] = result;

		return float(result);
	}
}
