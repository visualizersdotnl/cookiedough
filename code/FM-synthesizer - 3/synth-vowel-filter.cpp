
/*
	Syntherklaas FM -- Simple vowel (formant shaper) filter.

	This is a rather condensed/cheap way of implementing a formant shaper (which usually consists of
	bandpassing multiple formants more carefully) but very effective; and that is what it is: an effect.

	Mainly used to achieve certain degrees of distortion.
*/

// #include "synth-global.h"
#include "synth-vowel-filter.h"

const double kVowelCoeffs[SFM::VowelFilter::kNumVowels][11]= 
{
	// A

	/*
		from : antiprosynthesis[AT]hotmail[DOT]com
		comment : The distorting/sharp A vowel can be toned down easy by just changing the first coeff from 8.11044e-06 to 3.11044e-06. Sounds much better that way.

		Commented for now, took out too much volume.
	*/

	{ 
		8.11044e-06, // 3.11044e-06,
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
	float VowelFilter::Calculate(float sample, Vowel vowelA, unsigned iBuf)
	{
		SFM_ASSERT(vowelA < kNumVowels);
		SFM_ASSERT(iBuf < 2);

		double *buffer = m_ring[iBuf];
	
		/*
			from : stefan[DOT]hallen[AT]dice[DOT]se
			comment : Yeah, morphing lineary between the coefficients works just fine. The distortion I only get when not lowering the amplitude of the input. So I lower it :) Larsby, you can approximate filter curves quite easily, check your dsp literature :) 
		*/

		const double *vowelCoeffs = kVowelCoeffs[vowelA];

		double result;
		result  = vowelCoeffs[0]  * sample;
		result += vowelCoeffs[1]  * buffer[0];
		result += vowelCoeffs[2]  * buffer[1];
		result += vowelCoeffs[3]  * buffer[2];
		result += vowelCoeffs[4]  * buffer[3];
		result += vowelCoeffs[5]  * buffer[4];
		result += vowelCoeffs[6]  * buffer[5];
		result += vowelCoeffs[7]  * buffer[6];
		result += vowelCoeffs[8]  * buffer[7];
		result += vowelCoeffs[9]  * buffer[8];
		result += vowelCoeffs[10] * buffer[9];

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

		return Clamp((float) result);
	}

	float VowelFilter::Apply(float sample, Vowel vowelA, float delta)
	{
		if (0.f == delta)
			return Calculate(sample, vowelA, 0);

		const Vowel vowelB = Vowel( (vowelA+1) % kNumVowels );
		const float result = lerpf<float>(Calculate(sample, vowelA, 0), Calculate(sample, vowelB, 1), delta);
		return result;
	}
}
