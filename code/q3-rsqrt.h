
// cookiedough -- rsqrtf() implemention originating from Carmack/Quake 3 (see https://en.wikipedia.org/wiki/Fast_inverse_square_root)

#pragma once

VIZ_INLINE float Q_rsqrt(float number)
{
	union {
		float f;
		uint32_t i;
	} conv;
	
	float x2;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	conv.f  = number;
	conv.i  = 0x5f3759df - ( conv.i >> 1 );
	conv.f  = conv.f * ( threehalfs - ( x2 * conv.f * conv.f ) );
	return conv.f;
}
