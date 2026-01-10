
#pragma once

#if defined(_WIN32)

	#define FOR_INTEL
	#define DEBUG_TRAP __debugbreak();

#elif defined(__GNUC__) // traps both Linux and OSX for now

	#if defined(__ARM_NEON) || defined(__ARM_NEON__)
		#define SSE2NEON_SUPPRESS_WARNINGS
		#include "../3rdparty/sse2neon-stripped/sse2neon.h"
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

