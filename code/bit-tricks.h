
// cookiedough -- bit tricks

#ifndef _BIT_TRICKS_H_
#define _BIT_TRICKS_H_

CKD_INLINE static unsigned NonZeroMask32(int value)
{
	return -((value >> 31) ^ 1) & 0x7fffffff;
}

CKD_INLINE static uint32_t NonZeroMask32f(float value) {
	return NonZeroMask32(std::bit_cast<int32_t>(value));
}

CKD_INLINE static const bool IsPow2(unsigned value)
{
	return value != 0 && !(value & (value - 1));
}

CKD_INLINE static unsigned RoundPow2_32(unsigned value)
{
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	return value+1;
}

CKD_INLINE static size_t RoundPow2_64(size_t value)
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

#if defined(MSVC)
	#pragma warning(disable:4146) // unary minus operator applied, result still unsigned (IsZero())
#endif

CKD_INLINE static unsigned IsNotZero32(unsigned value) { 
	return ((value | (~value + 1)) >> 31); 
}

CKD_INLINE static unsigned IsZero32(unsigned value) { 
	return 1 + (value >> 31) - (-value >> 31); 
}

#if defined(MSVC)
	#pragma warning(enable(4146)
#endif

#endif // _BIT_TRICKS_H_
