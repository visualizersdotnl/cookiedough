
// cookiedough -- Win32 & Posix aligned alloc. & free

#ifndef _ALLOC_ALIGNED_H_
#define _ALLOC_ALIGNED_H_

#include <stdlib.h>

// inline void* mallocAligned(size_t size, size_t align);
// inline void  freeAligned(void* address);

#if defined(_WIN32)

__forceinline static void* mallocAligned(size_t size, size_t align) { 
	return _aligned_malloc(size, align); }

__forceinline static void freeAligned(void* address) {  
	_aligned_free(address); 
}

#elif defined(__GNUC__)

inline static void* mallocAligned(size_t size, size_t align) { 
	void* address = nullptr;
	posix_memalign(&address, align, size);
	return address;
}

inline static void freeAligned(void* address) { 
	free(address); 
}

#endif

#endif // _ALLOC_ALIGNED_H_
