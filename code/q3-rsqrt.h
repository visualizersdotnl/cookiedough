
// cookiedough -- the infamous rsqrtf() approximation

/*
	not all that useful anymore overall and definitely not for visual accuracy (it's horrible when for ex. calculating normals for lighting)
	
	- conceptual explanation: https://betterexplained.com/articles/understanding-quakes-fast-inverse-square-root/
	- kept around for nostalgia (fits this project)

	once the problem here was reduced to 'iX = constant - (iX>>1)' a fitting constant was approximated using an algorithm that
	adjusted the constant by minimizing error for a practical range of values (which I guess can be deduced to some degree by simply looking
	at what Carmack actually threw at it in Quake, if you've got the time for that sort of thing)
*/

#pragma once

#if defined(__GNUC__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

template<unsigned numIter>
CKD_INLINE static float Q3_rsqrtf(float x)
{
	static_assert(numIter > 0);

	const float half = 0.5f*x;

	// hint: the integer reinterpretation behaves roughly like log₂(x), so this line approximates x⁻¹ᐟ² in log space
	int iX = std::bit_cast<int>(x);  

	// this is the constant that makes a single Newton iteration converge fairly well
	iX = 0x5f3759df - (iX>>1); 

	// reinterpret back and iterate at least once (multiple would improve it a bit)
	x = std::bit_cast<float>(iX);   

	for (unsigned i = 0; i < numIter; ++i)
		x = x*(1.5f - half*x*x); 

	return x;
}

#if defined(__GNUC__)
	#pragma GCC diagnostic pop
#endif
