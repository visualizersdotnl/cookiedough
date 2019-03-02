
/*
	Syntherklaas FM: fast (co)sine.
*/

#pragma once

namespace SFM
{
	void InitializeFastCosine();

	// Period [0..1]
	float fastcosf(double x);
	SFM_INLINE float fastsinf(double x) { return fastcosf(x-0.25); }
};
