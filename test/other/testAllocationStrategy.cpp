// This benchmark tries to answer the question if linear containers (like array/vector) that
// ALLOCATE/CLEAR all memory upfront are faster in terms of future element access
// than these that use memory as needed.
// The spring engine is preallocating game for 32k units which is way above the actual use
// hence most allocated memory is wasted.
// This benchmark does not measure the cost of accessing new pages by OS,
// which is not important as number of units in the game grows slowly
// but rather tests whether a container that grew slowly in a non-temporal manner is slower to access.
//
// Each test is a combination of:
// * allocation strategy (none/reserve/resize on std::vector)
// * data write strategy:
//   - Upfront from begin() to end() of allocator
//   - Incremental which grows each allocator by one page size at once
// * data read strategy: 
//   - std::accumulate (sequential left fold access)
//   - std::reduce (unspecified access)
// * data type (e.g. size_t)

// Possible result explanations:
//
// if tests Upfront (createDataUpfront()) are faster than Incremental (createDataIncremental())
// that means that temporal locality of memory page allocation is important and containers
// should be accessed for the first time linearly.
//
// otherwise there's no need clear/touch allocated memory segments and in effect conserve OS memory

#include <benchmark/benchmark.h>
#include <vector>
#include <algorithm>
#include <numeric>

static const size_t PAGE_SIZE = 4096;
static const size_t MAX_PAGES = 500;
static const size_t ALLOCATORS = 50;

template <typename ElemT>
class AllocatorExample {
  public:
    AllocatorExample() {
        static_assert(PAGE_SIZE * MAX_PAGES * ALLOCATORS <= 2147483648, "allocated more than 2Gb");
        allocators.resize(ALLOCATORS);
    }


    // SECTION: allocation strategies
    void no_allocate() { }
    void reserve() {
        for (auto& alloc : allocators) {
            alloc.reserve(indexOfPage(MAX_PAGES));
        }
    }
    void resize() {
        for (auto& alloc : allocators) {
            alloc.resize(indexOfPage(MAX_PAGES));
        }
    }


    // SECTION: fill data strategies
    void createDataIncremental() {
        // fill first page of each allocator, then second page of each allocator...
        for (size_t i = 0; i < MAX_PAGES; ++i) {
             for (auto& alloc : allocators) {
                populatePageSize(alloc, i);
             }
        }
    }   
    void createDataUpfront() {
        // fill one allocator fully first, then proceed with others
        for (auto& alloc : allocators) {
            for (size_t i = 0; i < MAX_PAGES; ++i) {
                populatePageSize(alloc, i);
            }
        }
    }


    // SECTION: data transform
    ElemT accumulate() const {
        ElemT val{};
        for (auto& alloc : allocators) {
            val = std::accumulate(alloc.cbegin(), alloc.cend(), val);
        }
        return val;
    }
    ElemT reduce() const {
        ElemT val{};
        for (auto& alloc : allocators) {
            val = std::reduce(alloc.cbegin(), alloc.cend(), val);
        }
        return val;
    }

  private:
    void populatePageSize(std::vector<ElemT>& alloc, size_t page_id) {
        if (indexOfPage(page_id) >= alloc.size())
        {
            // reserved or not allocated so need to resize
            alloc.resize(indexOfPage(page_id+1));
        }

        std::generate(alloc.begin() + indexOfPage(page_id),
                      alloc.begin() + indexOfPage(page_id+1),
                      [&]() { return rand() % 100; }
                      );

        benchmark::ClobberMemory();
    }

    size_t indexOfPage(size_t page_id) {
        // return index to first element of page
        return PAGE_SIZE * page_id / sizeof(ElemT);
    }

    std::vector<std::vector<ElemT>> allocators;
};


template<typename elemT, typename voidMethodT, typename readDataT>
void AllocateFillAndRead(benchmark::State& state, voidMethodT resizeStrategy, voidMethodT fillDataStrategy, readDataT readDataStrategy, elemT elem) {
    AllocatorExample<elemT> alloc;
    (alloc.*resizeStrategy)();   // reserve full, resize full or nothing
    (alloc.*fillDataStrategy)(); // write random data to all allocators
    benchmark::ClobberMemory();

    for (auto _ : state) {
        elemT sum = (alloc.*readDataStrategy)(); // read previously written data from all allocators
        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }
}


// SECTION: grow each allocator to full size first
BENCHMARK_CAPTURE(AllocateFillAndRead, ReserveAlloc_Upfront_Accumulate_SizeT,
                  &AllocatorExample<size_t>::reserve,
                  &AllocatorExample<size_t>::createDataUpfront,
                  &AllocatorExample<size_t>::accumulate,
                  size_t{});


BENCHMARK_CAPTURE(AllocateFillAndRead, ResizeAlloc_Upfront_Accumulate_SizeT,
                  &AllocatorExample<size_t>::resize,
                  &AllocatorExample<size_t>::createDataUpfront,
                  &AllocatorExample<size_t>::accumulate,
                  size_t{});
/*
BENCHMARK_CAPTURE(AllocateFillAndRead, NoAlloc_Upfront_AccumulateSizeT,
                  &AllocatorExample<size_t>::no_allocate,
                  &AllocatorExample<size_t>::createDataUpfront,
                  &AllocatorExample<size_t>::accumulate,
                  size_t{});
*/
// SECTION: grow allocators by small steps
BENCHMARK_CAPTURE(AllocateFillAndRead, ReserveAlloc_Incremental_Accumulate_SizeT,
                  &AllocatorExample<size_t>::reserve,
                  &AllocatorExample<size_t>::createDataIncremental,
                  &AllocatorExample<size_t>::accumulate,
                  size_t{});

BENCHMARK_CAPTURE(AllocateFillAndRead, ResizeAlloc_Incremental_Accumulate_SizeT,
                  &AllocatorExample<size_t>::resize,
                  &AllocatorExample<size_t>::createDataIncremental,
                  &AllocatorExample<size_t>::accumulate,
                  size_t{});

BENCHMARK_CAPTURE(AllocateFillAndRead, NoAlloc_Incremental_Accumulate_SizeT,
                  &AllocatorExample<size_t>::no_allocate,
                  &AllocatorExample<size_t>::createDataIncremental,
                  &AllocatorExample<size_t>::accumulate,
                  size_t{});

/*
BENCHMARK_CAPTURE(AllocateFillAndRead, Resize_Upfront_Reduce_SizeT,
                  &AllocatorExample<size_t>::resize,
                  &AllocatorExample<size_t>::createDataUpfront,
                  &AllocatorExample<size_t>::reduce,
                  size_t{});

BENCHMARK_CAPTURE(AllocateFillAndRead, Resize_Incremental_Reduce_SizeT,
                  &AllocatorExample<size_t>::resize,
                  &AllocatorExample<size_t>::createDataIncremental,
                  &AllocatorExample<size_t>::reduce,
                  size_t{});
*/

BENCHMARK_MAIN();
