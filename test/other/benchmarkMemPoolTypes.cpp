#include "System/Log/ILog.h"
#include "System/MemPoolTypes.h"

#include <array>
#include <cstddef>
#include <vector>

#include <benchmark/benchmark.h>

namespace {
namespace objsizes {
// Allocators' page sizes are set to the largest object in particular category (e.g. Weapons)
// These are the page sizes that are commonly used in the engine
constexpr size_t micro = 64;   // CMatrix44f
constexpr size_t small = 128;  // SolidObjectGroundDecal
constexpr size_t medium = 752; // CWeapon,PlasmaRepulser
constexpr size_t large = 1472; // CFeature
} // namespace objsizes
} // namespace

template<size_t T> struct ArrayData {
	std::array<char, T> data = {};
};

template<typename TMempool> static void BenchStaticMemPoolAllocation(benchmark::State& state)
{
	static TMempool mempool;
	mempool.clear();

	constexpr size_t page_size = mempool.PAGE_SIZE();
	using AD = ArrayData<page_size>;
	std::vector<AD*> allocated;

	for (auto _: state) {
		if (!mempool.can_alloc()) {
			state.PauseTiming();
			mempool.clear();
			allocated.clear();
			state.ResumeTiming();
		}
		auto* obj = mempool.template alloc<AD>();
		benchmark::DoNotOptimize(obj);
		allocated.push_back(obj);
		benchmark::ClobberMemory();
	}
}

BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<1024, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<1024, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<1024, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<1024, objsizes::large>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<10240, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<10240, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<10240, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<10240, objsizes::large>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<102400, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<102400, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<102400, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<102400, objsizes::large>>);

BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<512, 16, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<512, 16, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<512, 16, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<512, 16, objsizes::large>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<1024, 32, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<1024, 32, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<1024, 32, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<1024, 32, objsizes::large>>);

template<typename TMempool> static void BenchStaticMemPoolAllocationDeallocation(benchmark::State& state)
{
	static TMempool mempool;
	mempool.clear();

	constexpr size_t page_size = mempool.PAGE_SIZE();
	using AD = ArrayData<page_size>;
	std::vector<AD*> allocated;

	for (auto _: state) {
		if (!mempool.can_alloc()) {
			for (auto* obj: allocated) {
				mempool.free(obj);
			}
			allocated.clear();
		}
		auto* obj = mempool.template alloc<AD>();
		benchmark::DoNotOptimize(obj);
		allocated.push_back(obj);
		benchmark::ClobberMemory();
	}
}

BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<1024, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<1024, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<1024, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<1024, objsizes::large>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<10240, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<10240, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<10240, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<10240, objsizes::large>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<102400, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<102400, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<102400, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<102400, objsizes::large>>);

BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<512, 16, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<512, 16, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<512, 16, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<512, 16, objsizes::large>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<1024, 32, objsizes::micro>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<1024, 32, objsizes::small>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<1024, 32, objsizes::medium>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<1024, 32, objsizes::large>>);

BENCHMARK_MAIN();
