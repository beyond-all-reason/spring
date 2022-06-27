#ifdef USE_MIMALLOC
    #include <mimalloc.h>
#else
    #ifdef _WIN32
        #include <malloc.h>
    #else
        #include <cstdlib>
    #endif
#endif

#include "SpringMem.h"


void* spring::AllocateAlignedMemory(size_t size, size_t alignment)
{
#ifdef USE_MIMALLOC
    return mi_aligned_alloc(alignment, size);
#else
    #ifdef _WIN32
        return _aligned_malloc(size, alignment);
    #else
        void* ptr = nullptr;
        posix_memalign(&ptr, alignment, size);
        return ptr;
    #endif
#endif
}

void spring::FreeAlignedMemory(void* ptr)
{
    if (ptr)
    {
    #ifdef USE_MIMALLOC
        return mi_free(ptr);
    #else
        #ifdef _WIN32
            _aligned_free(ptr);
        #else
            ::free(ptr);
        #endif
    #endif
    }
}

void* spring::malloc(size_t size)
{
#ifdef USE_MIMALLOC
    return mi_malloc(size);
#else
    return ::malloc(size);
#endif
}

void spring::free(void* _Block)
{
#ifdef USE_MIMALLOC
    mi_free(_Block);
#else
    ::free(_Block);
#endif
}
