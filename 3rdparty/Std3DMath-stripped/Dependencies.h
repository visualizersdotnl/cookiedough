
#pragma once

#ifdef _WIN32
	#define S3D_INLINE __forceinline
#else
	#define S3D_INLINE __inline
#endif

// CRT & STL:
#include <assert.h>
#include <string.h>  // memcpy()
#include <math.h>    // sinf(), cosf(), et cetera
#include <algorithm> // std::min, std::max
#include <cmath>     // std::truncf(), et cetera
#include <float.h>   // FLT_EPSILON
#include <array>
#include <tuple>

#if defined(_WIN32) // No Win32? Try SSE2NEON for ex.
	// Firstly to align Vector3/Vector4, and enable SIMD on some of these primitives.
	#include <xmmintrin.h>
#endif
