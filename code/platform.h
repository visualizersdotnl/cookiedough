
#pragma once

#if defined(_WIN32)

	#define FOR_INTEL
	#define DEBUG_TRAP __debugbreak();

	#if defined (_MSC_VER)

		// we're compiling with MSVC (use anywhere necessary)
		#define MSVC 

		// tell MSVC to shut up about it's well-intentioned safer CRT functions
		#define _CRT_SECURE_NO_WARNINGS

	#endif

#elif defined(__GNUC__) // traps both Linux and OSX for now

	#if defined(__ARM_NEON) || defined(__ARM_NEON__)
		#define SSE2NEON_SUPPRESS_WARNINGS
		#include "../3rdparty/sse2neon/sse2neon.h"
		#define FOR_ARM
	#else // Most likely x64
		#define FOR_INTEL
	#endif
	
	#include <signal.h>
	#define DEBUG_TRAP raise(SIGTRAP);

#endif

#if defined(FOR_INTEL)
	// SSE intrinsics
	#include <xmmintrin.h> // 1
	#include <emmintrin.h> // 2, 3
	#include <smmintrin.h> // 4
#endif

// size of cache line
#if defined(FOR_INTEL)
	// generally this holds true for x86/x64
	constexpr size_t kCacheLine = sizeof(size_t)<<3;
#elif defined(FOR_ARM)
	// If it's ARM, we're banking on Apple Silicon (M), for which this is true
	constexpr size_t kCacheLine = 128; 
#endif

// Good for (I)SSE and x64 in general, should work for ARM/NEON too
constexpr size_t kAlignTo = 16; 

// Fair lowest common denominator for Intel x64 & Apple Silicon (low-power cores)
constexpr size_t kCacheL1 = 65536; // 64KB
