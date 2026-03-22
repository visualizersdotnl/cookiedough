
// cookiedough -- the infamous rsqrtf() approximation

// - (incomplete) explanation: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
// - don't use this if you need to be visually accurate (it's quite terrible when you're calculating normals for lighting for example)
// - kept around for nostalgia (rather fitting given the scope of this project)

#pragma once

#if defined(__GNUC__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

CKD_INLINE static float Q3_rsqrtf(float x)
{
	const float half = 0.5f*x;

	// this is the initial guess (and the magic) that makes a single Newton iteration work (quasi)
	int iX = std::bit_cast<int>(x); 
	iX = 0x5f3759df - (iX>>1); 

	// reinterpret back and iterate once (multiple would improve it a bit)
	x = std::bit_cast<float>(iX);   
	x = x*(1.5f - half*x*x); 

	return x;
}

#if defined(__GNUC__)
	#pragma GCC diagnostic pop
#endif
