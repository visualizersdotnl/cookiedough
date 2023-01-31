
// cookiedough -- fast (co)sine (by Kusma/Excess)

#include "main.h"

constexpr unsigned kFastCosTabLog2Size = 10; // Equals size of 1024
constexpr unsigned kFastCosTabSize = (1 << kFastCosTabLog2Size);

alignas(kAlignTo) static double s_fastCosTab[kFastCosTabSize+1];

void InitializeFastCosine()
{
	for (unsigned i = 0; i < kFastCosTabSize+1; ++i)
	{
		const double phase = double(i) * ((k2PI/kFastCosTabSize));
		s_fastCosTab[i] = cos(phase); // fpuCos(phase);
	}
}

float fastcosf(double x)
{
	// Cosine is symmetrical around 0, let's get rid of negative values
	x = fabs(x); 

	// Convert [0..k2PI] to [1..2]
	const auto phaseScale = 1.f/k2PI;
	const auto phase = 1.0 + x*phaseScale;

	const auto phaseAsInt = *reinterpret_cast<const unsigned long long *>(&phase);
	const int exponent = (phaseAsInt >> 52) - 1023;

	constexpr auto fractBits = 32 - kFastCosTabLog2Size;
	constexpr auto fractScale = 1 << fractBits;
	constexpr auto fractMask = fractScale - 1;

	const auto significand = (unsigned int)((phaseAsInt << exponent) >> (52 - 32));
	const auto index = significand >> fractBits;
	const int fract = significand & fractMask;

	const auto left = s_fastCosTab[index];
	const auto right = s_fastCosTab[index+1];

	const auto fractMix = fract*(1.0/fractScale);
	return float(left + (right-left) * fractMix);
}
