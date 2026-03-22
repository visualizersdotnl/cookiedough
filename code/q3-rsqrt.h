
// cookiedough -- the rsqrtf() approximation that is chiefly a bit of 1990s lore by now

// - (incomplete) explanation: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
// - don't use this if you need to be accurate, plus on most modern hardware you'd be hard pressed to find a code path for which it's still worth it

#pragma once

#if defined(__GNUC__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

CKD_INLINE static float Q3_rsqrtf(float x)
{
	const float xhalf = 0.5f*x;
	int i = std::bit_cast<int>(x); 
	i = 0x5f3759df - (i >> 1); // this is the initial guess (and the magic) that makes a single Newton iteration work (quasi)
	x = std::bit_cast<float>(i);   
	x = x*(1.5f - xhalf*x*x);  // single iteration of Newton's method (obviously repeating this would improve things)
	return x;
}

#if defined(__GNUC__)
	#pragma GCC diagnostic pop
#endif