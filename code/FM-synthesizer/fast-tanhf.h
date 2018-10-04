
/*
	Syntherklaas FM
	
	Approx. hyperbolic tangent function, taken from:
	https://varietyofsound.wordpress.com/2011/02/14/efficient-tanh-computation-using-lamberts-continued-fraction/
*/

#ifndef _SFM_FAST_TANHF_H_
#define _SFM_FAST_TANHF_H_

#include "synth-global.h"

namespace SFM
{
	SFM_INLINE float fast_tanhf(float x) 
	{
//		return tanhf(x);

		const float xx = x*x;
		const float a = (((xx*378.f)*xx + 17325.f)*xx + 135135.f)*x;
		const float b = ((28.f*xx + 3150.f)*xx + 62370.f)*xx +135135.f;
		return a/b;
	}
}

#endif // _SFM_FAST_TANHF_H_
