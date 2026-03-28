
// cookiedough -- fast (co)sine (use with care!)

// important: 
// - linear domain [-1..1] (for PLLs)
// - quality degrades for large numbers, optimized for phase locked loops

#pragma once

constexpr unsigned kFastCosTabLog2Size = 10; // Equals size of 1024
constexpr unsigned kFastCosTabSize = (1 << kFastCosTabLog2Size);

extern double g_fastCosTab[kFastCosTabSize+1];

void InitializeFastCosine();

CKD_INLINE static float fastcosf(double x)
{
	// ignore sign since cos(-x) = cos(x)
	x = fabs(x);

	// shift to radians, offset by 1 to hopefully sit in IEEE754 mantissa normalized range [1..2]
	constexpr double phaseScale = 1.f/k2PI;
	const auto phase = 1.0 + x*phaseScale;

	// [ sign | exponent (11-bit) | mantissa (52-bit) ]
	const uint64_t phaseBits = std::bit_cast<uint64_t>(phase); 

	// extract exponent and remove bias
	const int exponent = int(phaseBits>>52)-1023; 

	// reconstruct a 32-bit wrapped fixed-point value,
	// then split into table index (high bits) and interpolation fraction (low bits)
	constexpr unsigned fractBits  = 32 - kFastCosTabLog2Size; // e.g. 22 bits
	constexpr unsigned fractScale = 1u << fractBits;
	constexpr unsigned fractMask  = fractScale - 1;

	// approximate linear (throw out lower bits, that's where we stand to lose accuracy)
	const uint32_t significand = uint32_t((phaseBits<<exponent) >> (52-32));

	// retrieve table index & sample LUT
    const unsigned index = significand>>fractBits;  
    const double left  = g_fastCosTab[index];
    const double right = g_fastCosTab[index+1];

	// interpolate, back to single prec., return
	const double t = double(significand&fractMask) * (1.0/fractScale);
    return float(left + (right-left)*t);
}

CKD_INLINE static float fastsinf(double x) {
	return fastcosf(x-0.25); 
}
