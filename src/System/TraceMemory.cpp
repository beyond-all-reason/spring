/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#if defined(TRACY_ENABLE)

#include <new>
#include <cstdlib>
#include <tracy/Tracy.hpp>

void* operator new(std::size_t count)
{
	auto ptr = malloc(count);
	TracyAlloc(ptr, count);
	return ptr;
}

void operator delete (void* ptr) noexcept
{
	TracyFree(ptr);
	free(ptr);
}

#endif
