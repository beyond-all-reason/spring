/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MemoryOverride.hpp"

#ifdef USE_MIMALLOC
#include <mimalloc.h>
#else
#include <cstring>
#include <cstdlib>
#include <cstdint>
#ifdef _WIN32
#include <malloc.h>
#else
#include <malloc.h>  // for malloc_usable_size on Linux
#endif
#endif

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

#include <algorithm>
#include <cassert>
#include <new>

// ----------------------------------------------------------------------------
// recoil namespace memory allocation functions
// ----------------------------------------------------------------------------

namespace recoil {

// malloc and free are inline in the header when USE_MIMALLOC is not enabled
#if defined(USE_MIMALLOC) && USE_MIMALLOC
void* malloc(size_t size)
{
	return mi_malloc(size);
}

void free(void* ptr)
{
	mi_free(ptr);
}
#endif

void* calloc(size_t count, size_t size)
{
#ifdef USE_MIMALLOC
	return mi_calloc(count, size);
#else
	return std::calloc(count, size);
#endif
}

void* realloc(void* ptr, size_t size)
{
#ifdef USE_MIMALLOC
	return mi_realloc(ptr, size);
#else
	return std::realloc(ptr, size);
#endif
}

void* aligned_alloc(size_t alignment, size_t size)
{
#ifdef USE_MIMALLOC
	return mi_aligned_alloc(alignment, size);
#else
	#if defined(_WIN32) || defined(__MINGW32__)
		return _aligned_malloc(size, alignment);
	#else
		// std::aligned_alloc requires size to be a multiple of alignment (C11/C++17);
		// unlike posix_memalign, passing a non-multiple is undefined behaviour
		size = (size + alignment - 1) & ~(alignment - 1);  // bitwise round-up
		assert(std::has_single_bit(alignment)); // assumption
		assert(size % alignment == 0); // intention
		return std::aligned_alloc(alignment, size);
	#endif
#endif
}

void* aligned_realloc(void* ptr, size_t oldsize, size_t newsize, size_t alignment)
{
	alignment = std::max<size_t>(alignment, 1);
#ifdef USE_MIMALLOC
	return mi_realloc_aligned(ptr, newsize, alignment);
#else
	#if defined(_WIN32) || defined(__MINGW32__)
		return _aligned_realloc(ptr, newsize, alignment);
	#else
		void* const tmp = realloc(ptr, newsize);
		if (!tmp)
			throw std::bad_alloc();
		ptr = tmp;
		if (reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0)
			return ptr;

		// bad luck: realloc didn't give us the right alignment
		void* newPtr = nullptr;
		if (posix_memalign(&newPtr, alignment, newsize) != 0) {
			free(ptr);
			throw std::bad_alloc();
		}
		std::memcpy(newPtr, ptr, std::min(oldsize, newsize));
		free(ptr);
		return newPtr;
	#endif
#endif
}

void aligned_free(void* ptr)
{
#ifdef USE_MIMALLOC
	mi_free(ptr);  // mimalloc handles aligned free automatically
#else
	#if defined(_WIN32) || defined(__MINGW32__)
		_aligned_free(ptr);
	#else
		std::free(ptr);
	#endif
#endif
}

size_t usable_size(void* ptr)
{
#ifdef USE_MIMALLOC
	return mi_usable_size(ptr);
#else
	#if defined(_WIN32) || defined(__MINGW32__)
		return _msize(ptr);
	#else
		return malloc_usable_size(ptr);
	#endif
#endif
}

} // namespace recoil

// ----------------------------------------------------------------------------
// Custom operator new/delete overrides using mimalloc
// This file should be compiled only once into the engine
// Based on mimalloc-new-delete.h from mimalloc distribution
// When TRACY_ENABLE is defined, also tracks allocations with Tracy
// ----------------------------------------------------------------------------

#ifdef USE_MIMALLOC

// C++98 delete operators
void operator delete(void* p) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free(p);
}

void operator delete[](void* p) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free(p);
}

// C++98 no-throw delete operators
void operator delete(void* p, const std::nothrow_t&) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free(p);
}

void operator delete[](void* p, const std::nothrow_t&) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free(p);
}

// C++98 new operators
void* operator new(std::size_t n) noexcept(false)
{
	void* p = mi_new(n);
#ifdef TRACY_ENABLE
	TracyAlloc(p, n);
#endif
	return p;
}

void* operator new[](std::size_t n) noexcept(false)
{
	void* p = mi_new(n);
#ifdef TRACY_ENABLE
	TracyAlloc(p, n);
#endif
	return p;
}

// C++98 no-throw new operators
void* operator new(std::size_t n, const std::nothrow_t& tag) noexcept
{
	(void)(tag);
	void* p = mi_new_nothrow(n);
#ifdef TRACY_ENABLE
	TracyAlloc(p, n);
#endif
	return p;
}

void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept
{
	(void)(tag);
	void* p = mi_new_nothrow(n);
#ifdef TRACY_ENABLE
	TracyAlloc(p, n);
#endif
	return p;
}

// C++14 sized deallocation operators
#if (__cplusplus >= 201402L || _MSC_VER >= 1916)
void operator delete(void* p, std::size_t n) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free_size(p, n);
}

void operator delete[](void* p, std::size_t n) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free_size(p, n);
}
#endif

// C++17 aligned new/delete operators
#if (__cplusplus > 201402L || defined(__cpp_aligned_new))
void operator delete(void* p, std::align_val_t al) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free_aligned(p, static_cast<size_t>(al));
}

void operator delete[](void* p, std::align_val_t al) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free_aligned(p, static_cast<size_t>(al));
}

void operator delete(void* p, std::size_t n, std::align_val_t al) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free_size_aligned(p, n, static_cast<size_t>(al));
}

void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free_size_aligned(p, n, static_cast<size_t>(al));
}

void operator delete(void* p, std::align_val_t al, const std::nothrow_t&) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free_aligned(p, static_cast<size_t>(al));
}

void operator delete[](void* p, std::align_val_t al, const std::nothrow_t&) noexcept
{
#ifdef TRACY_ENABLE
	TracyFree(p);
#endif
	mi_free_aligned(p, static_cast<size_t>(al));
}

void* operator new(std::size_t n, std::align_val_t al) noexcept(false)
{
	void* p = mi_new_aligned(n, static_cast<size_t>(al));
#ifdef TRACY_ENABLE
	TracyAlloc(p, n);
#endif
	return p;
}

void* operator new[](std::size_t n, std::align_val_t al) noexcept(false)
{
	void* p = mi_new_aligned(n, static_cast<size_t>(al));
#ifdef TRACY_ENABLE
	TracyAlloc(p, n);
#endif
	return p;
}

void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
	void* p = mi_new_aligned_nothrow(n, static_cast<size_t>(al));
#ifdef TRACY_ENABLE
	TracyAlloc(p, n);
#endif
	return p;
}

void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
	void* p = mi_new_aligned_nothrow(n, static_cast<size_t>(al));
#ifdef TRACY_ENABLE
	TracyAlloc(p, n);
#endif
	return p;
}
#endif

#endif // USE_MIMALLOC
