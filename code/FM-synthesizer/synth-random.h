
/*
	Syntherklaas FM -- 32-bit random generator (using TinyMT Mersenne-Twister).
*/

#pragma once

#include "synth-global.h"

namespace SFM
{
	void InitializeRandomGenerator();

	/*
		randf()   -- Returns a random value between not 0 but epsilon and 1 (convenient in case you want to divide).
		randu32() -- Unsigned 32-bit.
		rand32()  -- Signed 32-bit.
	*/

	float mt_randf();
	uint32_t mt_randu32();
	int32_t mt_rand32();
};
