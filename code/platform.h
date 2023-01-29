
#pragma once

#if defined(_WIN32)

	#define FOR_INTEL

#elif __GNUC__

	#if defined(__ARM_NEON) || defined(__ARM_NEON__)
		#include "../3rdparty/sse2neon/sse2neon.h"
		#define FOR_ARM
	#else // Most likely x64
		#define FOR_INTEL
	#endif

#endif

#if defined(FOR_INTEL)
	// SSE intrinsics
	#include <intrin.h>
	#include <xmmintrin.h> // 1
	#include <emmintrin.h> // 2, 3
	#include <smmintrin.h> // 4
#endif
