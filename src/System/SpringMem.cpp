#ifdef _WIN32
    #include <malloc.h>
#else
    #include <cstdlib>
    #include <cstdint>
    #include <cstring>
    #include <new>
#endif

#include "SpringMem.h"


void* spring::AllocateAlignedMemory(size_t size, size_t alignment)
{
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0)
        throw std::bad_alloc();
    return ptr;
#endif
}

void* spring::ReallocateAlignedMemory(void* ptr, size_t size, size_t alignment)
{
#ifdef _WIN32
    return _aligned_realloc(ptr, size, alignment);
#else
    void *const tmp = realloc(ptr, size);
    if (!tmp)
        throw std::bad_alloc();
    ptr = tmp;
    if (reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0)
        return ptr;

    // bad luck
    void* newPtr = nullptr;
    if (posix_memalign(&newPtr, alignment, size) != 0)
        throw std::bad_alloc();

    std::memcpy(ptr, newPtr, size);
    free(ptr);

    return newPtr;
#endif
}

void spring::FreeAlignedMemory(void* ptr)
{
    if (ptr)
    {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
}
