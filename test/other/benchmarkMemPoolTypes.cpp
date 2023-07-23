#include "System/MemPoolTypes.h"
#include "System/Log/ILog.h"

#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <vector>

template <size_t T>
struct ArrayData {
	char Data[T-8];
	size_t idx=0;
};

template <typename TMempool>
static void BenchStaticMemPoolAllocation(benchmark::State& state) {
	static TMempool mempool;
	mempool.clear();

	constexpr size_t page_size = mempool.PAGE_SIZE();
	using AD = ArrayData<page_size>;
	std::vector<AD*> allocated;

	size_t idx =0;
	for (auto _ : state) {
		if (!mempool.can_alloc()) {
			state.PauseTiming();
			mempool.clear();
			allocated.clear();
			state.ResumeTiming();
		}
		auto* obj = mempool.template alloc<AD>();
		obj->idx = ++idx;
		benchmark::DoNotOptimize(obj);
		allocated.push_back(obj);
		benchmark::ClobberMemory();
	}
}


BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<10240, 512>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<10240, 1024>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<102400, 512>>);
BENCHMARK(BenchStaticMemPoolAllocation<StaticMemPool<102400, 1024>>);

BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<512, 16, 256>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<1024, 32, 1024>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<512, 16, 256>>);
BENCHMARK(BenchStaticMemPoolAllocation<FixedDynMemPool<1024, 32, 1024>>);

template <typename TMempool>
static void BenchStaticMemPoolAllocationDeallocation(benchmark::State& state) {
	static TMempool mempool;
	mempool.clear();

	constexpr size_t page_size = mempool.PAGE_SIZE();
	using AD = ArrayData<page_size>;
	std::vector<AD*> allocated;

	size_t idx =0;
	for (auto _ : state) {
		if (!mempool.can_alloc()) {
			for (auto* obj : allocated) {
				mempool.free(obj);
			}
			allocated.clear();
		}
		auto* obj = mempool.template alloc<AD>();
		obj->idx = ++idx;
		benchmark::DoNotOptimize(obj);
		allocated.push_back(obj);
		benchmark::ClobberMemory();
	}
}

BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<10240, 512>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<10240, 1024>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<102400, 512>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<StaticMemPool<102400, 1024>>);

BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<512, 16, 256>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<1024, 32, 1024>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<512, 16, 256>>);
BENCHMARK(BenchStaticMemPoolAllocationDeallocation<FixedDynMemPool<1024, 32, 1024>>);

BENCHMARK_MAIN();
