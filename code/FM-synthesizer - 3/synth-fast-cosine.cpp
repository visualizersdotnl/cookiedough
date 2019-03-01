
/*
	Syntherklaas FM: fast (co)sine.

	Taken from Logicoma's WaveSabre, provided by Erik 'Kusma' Faye-Lund.

	FIXME: clean up!
*/

#pragma once

#include "synth-global.h"

/*
static __declspec(naked) double __vectorcall fpuCos(double x)
{
	__asm
	{
		sub esp, 8

		movsd mmword ptr [esp], xmm0
		fld qword ptr [esp]
		fcos
		fstp qword ptr [esp]
		movsd xmm0, mmword ptr [esp]

		add esp, 8

		ret
	}
}
*/

namespace SFM
{
	const unsigned kFastCosTabLog2Size = 10; // Equals size of 1024
	const unsigned kFastCosTabSize = (1 << kFastCosTabLog2Size);

	static double s_fastCosTab[kFastCosTabSize+1];

	void InitializeFastCos()
	{
		for (unsigned i = 0; i < kFastCosTabSize+1; ++i)
		{
			const double phase = double(i) * ((k2PI/kFastCosTabSize));
			s_fastCosTab[i] = cos(phase); // fpuCos(phase);
		}
	}

	float FastCos(double x)
	{
		x = fabs(x); // cosine is symmetrical around 0, let's get rid of negative values

		// normalize range from 0..2PI to 1..2
//		const auto phaseScale = 1.0/k2PI;
		auto phase = 1.0 + x; // * phaseScale;

		auto phaseAsInt = *reinterpret_cast<unsigned long long *>(&phase);
		int exponent = (phaseAsInt >> 52) - 1023;

		const auto fractBits = 32 - kFastCosTabLog2Size;
		const auto fractScale = 1 << fractBits;
		const auto fractMask = fractScale - 1;

		auto significand = (unsigned int)((phaseAsInt << exponent) >> (52 - 32));
		auto index = significand >> fractBits;
		int fract = significand & fractMask;

		auto left = s_fastCosTab[index];
		auto right = s_fastCosTab[index + 1];

		auto fractMix = fract * (1.0 / fractScale);
		return float(left + (right - left) * fractMix);
	}
};
