
// cookiedough -- rsqrtf() implemention (inspired) by Carmack (Quake III); be warned: not necessarily faster given the instructions surrounding this!

#pragma once

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
