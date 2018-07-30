
/* GR-1: random number generator(s) -- (C) Tasty Chips Electronics */

#if !defined(RANDOM_H)
#define RANDOM_H

#include <stdint.h>

void initialize_random_generator();

/* 
	32-bit Mersenne-Twister random functions. 

	mt_randf()   -- Returns a random value between not 0 but epsilon and 1 (convenient in case you want to divide).
	mt_randu32() -- 32-bit unsigned random integer.
	mt_rand32()  -- See below.
*/

float mt_randf();
uint32_t mt_randu32();
int32_t mt_rand32();

#endif // RANDOM_H
