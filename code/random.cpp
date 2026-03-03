
// cookiedough -- Mersenne-Twister random generator (not thread-safe)

#include "random.h"
// #include <time.h>

/* Include 'Tiny Mersenne-Twister' 32-bit/64-bit right here, since it's the only place we'll be using it. */
#include "../3rdparty/tinymt/tinymt32.c"
#include "../3rdparty/tinymt/tinymt64.c"

/* static */ tinymt32_t s_genState32;
/* static */ tinymt64_t s_genState64;

void initialize_random_generator()
{
	const uint32_t seed = 0xbadf00d;
	tinymt32_init(&s_genState32, seed);
	tinymt64_init(&s_genState64, mt_randu32());
}

double mt_rand_norm() {
	return tinymt64_generate_doubleOC(&s_genState64);
}

float mt_rand_norm_f() {
	return tinymt32_generate_floatOC(&s_genState32);
}

uint32_t mt_randu32() {
	return tinymt32_generate_uint32(&s_genState32);
}

int32_t mt_rand32() {  
	return (int32_t) mt_randu32();
}

uint64_t mt_randu64() {
	return tinymt64_generate_uint64(&s_genState64);
}

int64_t mt_rand64() {
	return (int64_t) mt_randu64();
}
