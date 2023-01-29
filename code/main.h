
// cookiedough -- main header, include on top of each .cpp!

#ifndef _MAIN_H_
#define _MAIN_H_

#include "platform.h"

#if defined(_WIN32)
	// ignore:
	#pragma warning(disable:4530)   // unwind semantics missing
	#define _CRT_SECURE_NO_WARNINGS // tell MSVC to shut up about it's well-intentioned *_s() functions
#endif

// CRT & STL
#include <stdint.h>
#include <math.h>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <thread>

// OpenMP
#include <omp.h>

// list of industry aspect ratios
#include "../3rdparty/aspectratios.h"

// output resolution
constexpr size_t kResX = 1280;
constexpr size_t kResY = 720;
constexpr size_t kHalfResX = kResX/2;
constexpr size_t kHalfResY = kResY/2;
constexpr size_t kOutputSize = kResX*kResY;
constexpr size_t kOutputBytes = kOutputSize*sizeof(uint32_t);
constexpr float kAspect = (float)kResY/kResX;

// set description on failure (reported on shutdown)
void SetLastError(const std::string &description);

// basic utilities (memory, graphics, ISSE et cetera)
#include "util.h"

// (few) shared resources
#include "shared-resources.h"

#endif // _MAIN_H_
