
/*
	Syntherklaas FM: fast (co)sine.

	** Taken from Logicoma's WaveSabre, provided by Erik 'Kusma' Faye-Lund. **
*/

#pragma once

#include "synth-global.h"

/*
	This is used in WaveSabre but seeing as I build for 64-bit I can't use it as-is now.

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

		// Convert [0..1] to [1..2]
		auto phase = 1.0+x;

		auto phaseAsInt = *reinterpret_cast<unsigned long long *>(&phase);
		int exponent = (phaseAsInt >> 52) - 1023;

		const auto fractBits = 32 - kFastCosTabLog2Size;
		const auto fractScale = 1 << fractBits;
		const auto fractMask = fractScale - 1;

		auto significand = (unsigned int)((phaseAsInt << exponent) >> (52 - 32));
		auto index = significand >> fractBits;
		int fract = significand & fractMask;

		auto left = s_fastCosTab[index];
		auto right = s_fastCosTab[index+1];

		auto fractMix = fract*(1.0/fractScale);
		return float(left + (right-left) * fractMix);
	}
};
