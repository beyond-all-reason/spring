/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <vector>

#ifdef USE_MIMALLOC
#include <mimalloc.h>
#endif

namespace {

constexpr size_t KB = 1024;

namespace sizes {
	constexpr size_t tiny = 16;
	constexpr size_t small = 64;
	constexpr size_t medium = 256;
	constexpr size_t large = 1024;
	constexpr size_t huge = 64 * KB;
}

}

TEST_CASE("AllocFree_Tiny", "[benchmark]") {
	BENCHMARK("default") {
		void* ptr = std::malloc(sizes::tiny);
		std::free(ptr);
		return ptr;
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		void* ptr = mi_malloc(sizes::tiny);
		mi_free(ptr);
		return ptr;
	};
#endif
}

TEST_CASE("AllocFree_Small", "[benchmark]") {
	BENCHMARK("default") {
		void* ptr = std::malloc(sizes::small);
		std::free(ptr);
		return ptr;
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		void* ptr = mi_malloc(sizes::small);
		mi_free(ptr);
		return ptr;
	};
#endif
}

TEST_CASE("AllocFree_Medium", "[benchmark]") {
	BENCHMARK("default") {
		void* ptr = std::malloc(sizes::medium);
		std::free(ptr);
		return ptr;
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		void* ptr = mi_malloc(sizes::medium);
		mi_free(ptr);
		return ptr;
	};
#endif
}

TEST_CASE("AllocFree_Large", "[benchmark]") {
	BENCHMARK("default") {
		void* ptr = std::malloc(sizes::large);
		std::free(ptr);
		return ptr;
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		void* ptr = mi_malloc(sizes::large);
		mi_free(ptr);
		return ptr;
	};
#endif
}

TEST_CASE("AllocFree_Huge", "[benchmark]") {
	BENCHMARK("default") {
		void* ptr = std::malloc(sizes::huge);
		std::free(ptr);
		return ptr;
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		void* ptr = mi_malloc(sizes::huge);
		mi_free(ptr);
		return ptr;
	};
#endif
}

TEST_CASE("AllocFreeBatch_Small", "[benchmark]") {
	constexpr size_t batch_size = 1000;
	std::vector<void*> ptrs(batch_size);

	BENCHMARK("default") {
		for (size_t i = 0; i < batch_size; ++i) {
			ptrs[i] = std::malloc(sizes::small);
		}
		for (size_t i = 0; i < batch_size; ++i) {
			std::free(ptrs[i]);
		}
		return ptrs[0];
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		for (size_t i = 0; i < batch_size; ++i) {
			ptrs[i] = mi_malloc(sizes::small);
		}
		for (size_t i = 0; i < batch_size; ++i) {
			mi_free(ptrs[i]);
		}
		return ptrs[0];
	};
#endif
}

TEST_CASE("AllocFreeBatch_Medium", "[benchmark]") {
	constexpr size_t batch_size = 1000;
	std::vector<void*> ptrs(batch_size);

	BENCHMARK("default") {
		for (size_t i = 0; i < batch_size; ++i) {
			ptrs[i] = std::malloc(sizes::medium);
		}
		for (size_t i = 0; i < batch_size; ++i) {
			std::free(ptrs[i]);
		}
		return ptrs[0];
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		for (size_t i = 0; i < batch_size; ++i) {
			ptrs[i] = mi_malloc(sizes::medium);
		}
		for (size_t i = 0; i < batch_size; ++i) {
			mi_free(ptrs[i]);
		}
		return ptrs[0];
	};
#endif
}

TEST_CASE("AllocFreeBatch_Large", "[benchmark]") {
	constexpr size_t batch_size = 1000;
	std::vector<void*> ptrs(batch_size);

	BENCHMARK("default") {
		for (size_t i = 0; i < batch_size; ++i) {
			ptrs[i] = std::malloc(sizes::large);
		}
		for (size_t i = 0; i < batch_size; ++i) {
			std::free(ptrs[i]);
		}
		return ptrs[0];
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		for (size_t i = 0; i < batch_size; ++i) {
			ptrs[i] = mi_malloc(sizes::large);
		}
		for (size_t i = 0; i < batch_size; ++i) {
			mi_free(ptrs[i]);
		}
		return ptrs[0];
	};
#endif
}

TEST_CASE("AllocFreeRandom_Mixed", "[benchmark]") {
	constexpr size_t count = 1000;
	std::vector<void*> ptrs(count);
	std::vector<size_t> alloc_sizes(count);

	std::mt19937_64 rng(42);
	std::uniform_int_distribution<size_t> dist(sizes::tiny, sizes::huge);

	for (size_t i = 0; i < count; ++i) {
		alloc_sizes[i] = dist(rng);
	}

	BENCHMARK("default") {
		for (size_t i = 0; i < count; ++i) {
			ptrs[i] = std::malloc(alloc_sizes[i]);
		}
		for (size_t i = 0; i < count; ++i) {
			std::free(ptrs[i]);
		}
		return ptrs[0];
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		for (size_t i = 0; i < count; ++i) {
			ptrs[i] = mi_malloc(alloc_sizes[i]);
		}
		for (size_t i = 0; i < count; ++i) {
			mi_free(ptrs[i]);
		}
		return ptrs[0];
	};
#endif
}

TEST_CASE("Calloc_Small", "[benchmark]") {
	BENCHMARK("default") {
		void* ptr = std::calloc(1, sizes::small);
		std::free(ptr);
		return ptr;
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		void* ptr = mi_calloc(1, sizes::small);
		mi_free(ptr);
		return ptr;
	};
#endif
}

TEST_CASE("Realloc_SmallToLarge", "[benchmark]") {
	BENCHMARK("default") {
		void* ptr = std::malloc(sizes::small);
		ptr = std::realloc(ptr, sizes::large);
		std::free(ptr);
		return ptr;
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		void* ptr = mi_malloc(sizes::small);
		ptr = mi_realloc(ptr, sizes::large);
		mi_free(ptr);
		return ptr;
	};
#endif
}

TEST_CASE("AlignedAlloc_64", "[benchmark]") {
	constexpr size_t alignment = 64;
#if defined(_WIN32) || defined(__MINGW32__)
	BENCHMARK("default") {
		void* ptr = _aligned_malloc(sizes::large, alignment);
		_aligned_free(ptr);
		return ptr;
	};
#else
	BENCHMARK("default") {
		void* ptr = std::aligned_alloc(alignment, sizes::large);
		std::free(ptr);
		return ptr;
	};
#endif
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		void* ptr = mi_aligned_alloc(alignment, sizes::large);
		mi_free(ptr);
		return ptr;
	};
#endif
}

TEST_CASE("MixedWorkload", "[benchmark]") {
	constexpr size_t iterations = 500;
	std::vector<void*> tiny_ptrs;
	std::vector<void*> medium_ptrs;
	std::vector<void*> large_ptrs;
	tiny_ptrs.reserve(iterations);
	medium_ptrs.reserve(iterations);
	large_ptrs.reserve(iterations);

	BENCHMARK("default") {
		for (size_t i = 0; i < iterations; ++i) {
			tiny_ptrs.push_back(std::malloc(sizes::tiny));
			medium_ptrs.push_back(std::malloc(sizes::medium));
			large_ptrs.push_back(std::malloc(sizes::large));
		}
		for (void* p : tiny_ptrs) std::free(p);
		for (void* p : medium_ptrs) std::free(p);
		for (void* p : large_ptrs) std::free(p);
		tiny_ptrs.clear();
		medium_ptrs.clear();
		large_ptrs.clear();
		return reinterpret_cast<void*>(0);
	};
#ifdef USE_MIMALLOC
	BENCHMARK("mimalloc") {
		for (size_t i = 0; i < iterations; ++i) {
			tiny_ptrs.push_back(mi_malloc(sizes::tiny));
			medium_ptrs.push_back(mi_malloc(sizes::medium));
			large_ptrs.push_back(mi_malloc(sizes::large));
		}
		for (void* p : tiny_ptrs) mi_free(p);
		for (void* p : medium_ptrs) mi_free(p);
		for (void* p : large_ptrs) mi_free(p);
		tiny_ptrs.clear();
		medium_ptrs.clear();
		large_ptrs.clear();
		return reinterpret_cast<void*>(0);
	};
#endif
}