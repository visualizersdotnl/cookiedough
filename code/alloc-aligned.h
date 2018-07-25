
// cookiedough -- Win32+GCC aligned alloc. & free

#ifndef _ALLOC_ALIGNED_H_
#define _ALLOC_ALIGNED_H_

#include <stdlib.h>

inline void* mallocAligned(size_t size, size_t align);
inline void  freeAligned(void* address);

#ifdef _WIN32

	inline void* mallocAligned(size_t size, size_t align) { return _aligned_malloc(size, align); }
	inline void  freeAligned(void* address) {  _aligned_free(address); }

#elif defined(__GNUC__)

	inline void* mallocAligned(size_t size, size_t align) 
	{ 
		void* address;
		posix_memalign(&address, align, size);
		return address;
	}

	inline void freeAligned(void* address) { free(address); }

#endif

#endif // _ALLOC_ALIGNED_H_
