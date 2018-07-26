
// cookiedough -- bit tricks

#ifndef _BIT_TRICKS_H_
#define _BIT_TRICKS_H_

// unsigned integer is-power-of-2 check
VIZ_INLINE const bool IsPow2(unsigned value)
{
	return value != 0 && !(value & (value - 1));
}

VIZ_INLINE unsigned RoundPow2_32(unsigned value)
{
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	return value+1;
}

#ifdef _WIN64

VIZ_INLINE size_t RoundPow2_64(size_t value)
{
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value |= value >> 32;
	return value+1;
}

#endif

#pragma warning(disable:4146) // unary minus operator applied, result still unsigned (IsZero())

// Thank you Bit Twiddling Hacks.
VIZ_INLINE unsigned IsNotZero(unsigned value) { return ((value | (~value + 1)) >> 31) & 1; }
VIZ_INLINE unsigned IsZero(unsigned value) { return 1 + (value >> 31) - (-value >> 31); }

#endif // _BIT_TRICKS_H_