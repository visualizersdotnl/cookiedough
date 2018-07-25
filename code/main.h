
// cookiedough -- main header, include on top of each .cpp!

#ifndef _MAIN_H_
#define _MAIN_H_

// ignore:
#pragma warning(disable:4530) // unwind semantics missing

// CRT & STL
#include <stdint.h>
#include <math.h>
#include <string>
#include <vector>
#include <memory>

// SSE intrinsics
#include <xmmintrin.h> // 1
#include <emmintrin.h> // 2, 3
#include <smmintrin.h> // 4

// output resolution
#define kResX 640
#define kResY 480
#define kHalfResX (kResX/2)
#define kHalfResY (kResY/2)

constexpr size_t kOutputSize = kResX*kResY;
constexpr size_t kOutputBytes = kOutputSize*sizeof(uint32_t);

// set description on failure (reported on shutdown)
void SetLastError(const std::string &description);

// basic utilities (memory, graphics, ...)
#include "util.h"

#endif // _MAIN_H_
