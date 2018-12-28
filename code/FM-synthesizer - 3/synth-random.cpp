
/*
	Syntherklaas FM -- 32-bit random generator (using TinyMT Mersenne-Twister).

	FIXME: uses Bevacqua's identical implementation for now
*/

#include "synth-global.h"
#include "synth-random.h"

/* Include 'Tiny Mersenne-Twister' 32-bit right here, since it's the only place we'll be using it. */
// #include "3rdparty/tinymt/tinymt32.c"

namespace SFM
{
//	static tinymt32_t s_genState;

	void InitializeRandomGenerator()
	{
//		const uint32_t seed = 0xbadf00d;
//		tinymt32_init(&s_genState, seed);
	}

	float mt_randf()
	{
//		return tinymt32_generate_floatOC(&s_genState);
		return ::mt_randf();
	}

	uint32_t mt_randu32()
	{
//		return tinymt32_generate_uint32(&s_genState);
		return ::mt_randu32();
	}

	int32_t mt_rand32() 
	{ 
//		return (int32_t) randu32();
		return ::mt_rand32();
	}
}
