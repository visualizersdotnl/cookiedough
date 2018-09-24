
/*
	Syntherklaas: FM synthesizer prototype.
	Fast(er) tanhf().
*/

#pragma once

// Source: https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
VIZ_INLINE float FM_taylor_tanhf(float x) 
{
	const float xx = x*x;
	const float a = (((xx*378.f)*xx + 17325.f)*xx + 135135.f)*x;
	const float b = ((28.f*xx + 3150.f)*xx + 62370.f)*xx +135135.f;
	return a/b;
}
