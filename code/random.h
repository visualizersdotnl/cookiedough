
// cookiedough -- Mersenne-Twister random generator

// FIXME (see Github issue): 
// - this isn't thread-safe (pulled from the GR-1 codebase)
// - as of 03/03/2026 that's not an issue, but it can be given the heavy OpenMP parallelization

#if !defined(RANDOM_H)
#define RANDOM_H

#include "../3rdparty/tinymt/tinymt32.h"
#include "../3rdparty/tinymt/tinymt64.h"

void initialize_random_generator();

// these return between epsilon and one, never zero
float mt_rand_norm_f();
double mt_rand_norm();

uint32_t mt_randu32();
int32_t mt_rand32();

uint64_t mt_randu64();
int64_t mt_rand64();

#endif // RANDOM_H
