
// cookiedough -- rsqrtf() implemention (inspired) by Carmack (Quake III)

#pragma once

/*

// source: https://en.wikipedia.org/wiki/Fast_inverse_square_root
VIZ_INLINE float Q3_rsqrtf(float number)
{
	union {
		float f;
		uint32_t i;
	} conv;
	
	float x2;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	conv.f  = number;
	conv.i  = 0x5f3759df - ( conv.i >> 1 );
	conv.f  = conv.f * ( threehalfs - ( x2 * conv.f * conv.f ) );
	return conv.f;
}
*/

// source: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
VIZ_INLINE float Q3_rsqrtf(float x)
{
	float xhalf = 0.5f * x;
	int i = *(int*)&x;            // store floating-point bits in integer
	i = 0x5f3759df - (i >> 1);    // initial guess for Newton's method
	x = *(float*)&i;              // convert new bits into float
	x = x*(1.5f - xhalf*x*x);     // One round of Newton's method
	return x;
}
