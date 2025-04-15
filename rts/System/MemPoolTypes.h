/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MEMPOOL_TYPES_H
#define MEMPOOL_TYPES_H

#include <cassert>
#include <cstddef>
#include <cstring> // memset
#include <cmath>
#include <array>
#include <deque>
#include <vector>
#include <map>
#include <memory>

#include "smmalloc/smmalloc.h"

#include "System/UnorderedMap.hpp"
#include "System/ContainerUtil.h"
#include "System/SafeUtil.h"
#include "System/Platform/Threading.h"
#include "System/Log/ILog.h"

template<uint32_t NumBuckets, size_t BucketSize> struct PassThroughPool {
public:
	PassThroughPool() {
		space = _sm_allocator_create(NumBuckets, BucketSize);
	}
	~PassThroughPool() {
		_sm_allocator_destroy(space); //checks space != nullptr internally
	}

	template<typename T, typename... A> T* alloc(A&&... a) {
		static_assert(BUCKET_STEP >= alignof(T), "Can't allocate memory with alignment greater than BUCKET_STEP");
		return new (allocMem(sizeof(T))) T(std::forward<A>(a)...);
	}
	void* allocMem(size_t size) {
		return _sm_malloc(space, size, BUCKET_STEP);
	}

	template<typename T> void free(T*& p) {
		void* m = p;

		spring::SafeDestruct(p);
		freeMem(m);
	}
	void freeMem(void* p) {
		_sm_free(space, p);
	}

	void* reAllocMem(void* p, size_t size) {
		return _sm_realloc(space, p, size, BUCKET_STEP);
	}

	bool isAllocInternal(void* p) const {
		return (space->GetBucketIndex(p) != -1);
	}
private:
	static constexpr size_t BUCKET_STEP = 16;
	static constexpr size_t INTERNAL_ALLOC_SIZE = BUCKET_STEP * NumBuckets;
	sm_allocator space = nullptr;
};

// Helper to infer the memory alignment and size from a set of types.
template <class ...T>
#if 0 // doesn't compile on MSVC 19.37
struct TypesMem {
    alignas(alignof(T)...) uint8_t data[std::max({sizeof(T)...})];
};
#else
using TypesMem = std::aligned_storage_t< std::max({ sizeof(T)... }), std::max({ alignof(T)... }) >;
#endif

template<size_t S, size_t Alignment> struct DynMemPool {
public:
	void* allocMem(size_t size) {
		assert(size <= PAGE_SIZE());
		uint8_t* m = nullptr;

		size_t i = 0;

		if (indcs.empty()) {
			pages.emplace_back();

			i = pages.size() - 1;
		} else {
			// must pop before ctor runs; objects can be created recursively
			i = spring::VectorBackPop(indcs);
		}

		m = pages[curr_page_index = i].data;

		table.emplace(m, i);
		return m;
	}


	template<typename T, typename... A> T* alloc(A&&... a) {
		static_assert(sizeof(T) <= PAGE_SIZE(), "");
		static_assert(Alignment >= alignof(T), "Memory pool memory is not sufficiently aligned");
		return new (allocMem(sizeof(T))) T(std::forward<A>(a)...);
	}


	void freeMem(void* m) {
		assert(mapped(m));

		const auto iter = table.find(m);
		const auto pair = std::pair<void*, size_t>{iter->first, iter->second};

		std::memset(pages[pair.second].data, 0, PAGE_SIZE());

		indcs.push_back(pair.second);
		table.erase(pair.first);
	}


	template<typename T> void free(T*& p) {
		assert(mapped(p));
		void* m = p;

		spring::SafeDestruct(p);
		// must free after dtor runs, since that can trigger *another* ctor call
		// by proxy (~CUnit -> ~CObject -> DependentDied -> CommandAI::FinishCmd
		// -> CBuilderCAI::ExecBuildCmd -> UnitLoader::LoadUnit -> CUnit e.g.)
		freeMem(m);
	}

	static constexpr size_t PAGE_SIZE() { return S; }

	size_t alloc_size() const { return (pages.size() * PAGE_SIZE()); } // size of total number of pages added over the pool's lifetime
	size_t freed_size() const { return (indcs.size() * PAGE_SIZE()); } // size of number of pages that were freed and are awaiting reuse

	bool mapped(void* p) const { return (table.find(p) != table.end()); }
	bool alloced(void* p) const { return ((curr_page_index < pages.size()) && (pages[curr_page_index].data == p)); }
	bool can_alloc() const { return true; }
	bool can_free() const { return indcs.size() < pages.size(); }

	void clear() {
		pages.clear();
		indcs.clear();
		table.clear();

		curr_page_index = 0;
	}
	void reserve(size_t n) {
		indcs.reserve(n);
		table.reserve(n);
	}

private:
	struct page {
		alignas(Alignment) uint8_t data[S];
	};

	std::deque<page> pages;
	std::vector<size_t> indcs;

	// <pointer, page index> (non-intrusive)
	spring::unsynced_map<void*, size_t> table;

	size_t curr_page_index = 0;
};

// Helper to infer the DynMemPool pool parameters from a types.
template<class ...T>
using DynMemPoolT = DynMemPool<sizeof(TypesMem<T...>), alignof(TypesMem<T...>)>;

// fixed-size dynamic version
// page size per chunk, number of chunks, number of pages per chunk
// at most <N * K> simultaneous allocations can be made from a pool
// of size NxK, each of which consumes S bytes (N chunks with every
// chunk consuming S * K bytes) excluding overhead
template<size_t S, size_t N, size_t K, size_t Alignment> struct FixedDynMemPool {
public:
	template<typename T, typename... A> T* alloc(A&&... a) {
		static_assert(sizeof(T) <= PAGE_SIZE(), "");
		static_assert(Alignment >= alignof(T), "Memory pool memory is not sufficiently aligned");
		return (new (allocMem(sizeof(T))) T(std::forward<A>(a)...));
	}

	void* allocMem(size_t size) {
		if (indcs.empty()) {
			// pool is full
			if (num_chunks == N)
				return nullptr;

			assert(chunks[num_chunks] == nullptr);
			chunks[num_chunks].reset(new t_chunk_mem());

			// reserve new indices; in reverse order since each will be popped from the back
			indcs.reserve(K);

			for (size_t j = 0; j < K; j++) {
				indcs.push_back(static_cast<uint32_t>((num_chunks + 1) * K - j - 1));
			}

			num_chunks += 1;
		}

		const uint32_t idx = spring::VectorBackPop(indcs);

		assert(size <= PAGE_SIZE());
		t_page_mem* page = page_mem(idx);
		page_index = page->index = idx;
		return page->data;
	}


	template<typename T> void free(T*& ptr) {
		static_assert(sizeof(T) <= PAGE_SIZE(), "");

		T* tmp = ptr;

		spring::SafeDestruct(ptr);
		freeMem(tmp);
	}

	void freeMem(void* ptr) {
		t_page_mem* page = page_mem_from_ptr(ptr);
		assert(page->index < (N * K));
		indcs.push_back(page->index);
		memset(page, 0, sizeof(t_page_mem));
	}

	void reserve(size_t n) { indcs.reserve(n); }
	void clear() {
		indcs.clear();

		// for every allocated chunk, add back all indices
		// (objects are assumed to have already been freed)
		for (size_t i = 0; i < num_chunks; i++) {
			for (size_t j = 0; j < K; j++) {
				indcs.push_back(static_cast<uint32_t>((i + 1) * K - j - 1));
			}
		}

		page_index = 0;
	}

	static constexpr size_t NUM_CHUNKS() { return N; } // size K*S
	static constexpr size_t NUM_PAGES() { return K; } // per chunk
	static constexpr size_t PAGE_SIZE() { return S; }

	size_t alloc_size() const { return (num_chunks * NUM_PAGES() * PAGE_SIZE()); } // size of total number of pages added over the pool's lifetime
	size_t freed_size() const { return (indcs.size() * PAGE_SIZE()); } // size of number of pages that were freed and are awaiting reuse

	bool mapped(void* ptr) const { return ((page_mem_from_ptr(ptr)->index < (num_chunks * K)) && (page_mem(page_mem_from_ptr(ptr)->index)->data == ptr)); }
	bool alloced(void* ptr) const { return ((page_index < (num_chunks * K)) && (page_mem(page_index)->data == ptr)); }
	bool can_alloc() const { return num_chunks < N || !indcs.empty() ; }
	bool can_free() const { return indcs.size() < (NUM_CHUNKS() * NUM_PAGES()); }

private:
	struct t_page_mem {
		uint32_t index;
		alignas(Alignment) uint8_t data[S];
	};

	typedef std::array<t_page_mem, K> t_chunk_mem;

	const t_page_mem* page_mem(size_t idx) const {
		return &(*chunks[idx / K])[idx % K];
	}

	t_page_mem* page_mem(size_t idx) {
		return &(*chunks[idx / K])[idx % K];
	}

	const t_page_mem* page_mem_from_ptr(void* ptr) const {
		return reinterpret_cast<const t_page_mem*>(reinterpret_cast<const uint8_t*>(ptr) - offsetof(t_page_mem, data));
	}

	t_page_mem* page_mem_from_ptr(void* ptr) {
		return reinterpret_cast<t_page_mem*>(reinterpret_cast<uint8_t*>(ptr) - offsetof(t_page_mem, data));
	}

	std::array<std::unique_ptr<t_chunk_mem>, N> chunks;
	std::vector<uint32_t> indcs;

	size_t num_chunks = 0;
	size_t page_index = 0;
};

// Helper to infer the FixedDynMemPool pool parameters from a types.
template<size_t N, size_t K, class ...T>
using FixedDynMemPoolT = FixedDynMemPool<sizeof(TypesMem<T...>), N, K, alignof(TypesMem<T...>)>;

// fixed-size version.
template<size_t N, size_t S, size_t Alignment> struct StaticMemPool {
public:
	StaticMemPool() { clear(); }

	void* allocMem(size_t size) {
		assert(size <= PAGE_SIZE());
		static_assert(NUM_PAGES() != 0, "");

		size_t i = 0;

		assert(can_alloc());

		if (free_page_count == 0) {
			i = used_page_count++;
		} else {
			i = indcs[--free_page_count];
		}

		return (pages[curr_page_index = i].data());
	}


	template<typename T, typename... A> T* alloc(A&&... a) {
		static_assert(sizeof(T) <= PAGE_SIZE(), "");
		static_assert(Alignment >= alignof(T), "Memory pool memory is not sufficiently aligned");
		return new (allocMem(sizeof(T))) T(std::forward<A>(a)...);
	}

	void freeMem(void* m) {
		assert(can_free());
		assert(mapped(m));

		std::memset(m, 0, PAGE_SIZE());

		// mark page as free
		indcs[free_page_count++] = base_offset(m) / PAGE_SIZE();
	}


	template<typename T> void free(T*& p) {
		assert(mapped(p));
		void* m = p;

		spring::SafeDestruct(p);
		freeMem(m);
	}


	static constexpr size_t NUM_PAGES() { return N; }
	static constexpr size_t PAGE_SIZE() { return S; }

	size_t alloc_size() const { return (used_page_count * PAGE_SIZE()); } // size of total number of pages added over the pool's lifetime
	size_t freed_size() const { return (free_page_count * PAGE_SIZE()); } // size of number of pages that were freed and are awaiting reuse
	size_t total_size() const { return (NUM_PAGES() * PAGE_SIZE()); }
	size_t base_offset(const void* p) const { return (reinterpret_cast<const uint8_t*>(p) - reinterpret_cast<const uint8_t*>(pages[0].data())); }

	bool mapped(const void* p) const { return (((base_offset(p) / PAGE_SIZE()) < total_size()) && ((base_offset(p) % PAGE_SIZE()) == 0)); }
	bool alloced(const void* p) const { return (pages[curr_page_index].data() == p); }

	bool can_alloc() const { return (used_page_count < NUM_PAGES() || free_page_count > 0); }
	bool can_free() const { return (free_page_count < NUM_PAGES()); }

	void reserve(size_t) {} // no-op
	void clear() {
		std::memset(pages.data(), 0, total_size());
		std::memset(indcs.data(), 0, NUM_PAGES());

		used_page_count = 0;
		free_page_count = 0;
		curr_page_index = 0;
	}

private:
	alignas(Alignment) std::array<std::array<uint8_t, S>, N> pages;
	std::array<size_t, N> indcs;

	size_t used_page_count = 0;
	size_t free_page_count = 0; // indcs[fpc-1] is the last recycled page
	size_t curr_page_index = 0;
};

// Helper to infer the StaticMemPool pool parameters from a types.
template<size_t N, class ...T>
using StaticMemPoolT = StaticMemPool<N, sizeof(TypesMem<T...>), alignof(TypesMem<T...>)>;


// dynamic memory allocator operating with stable index positions
// has gaps management
template <typename T>
class StablePosAllocator {
public:
	using Type = T;
public:
	static constexpr bool reportWork = false;
	template<typename ...Args>
	static void myLog(Args&&... args) {
		if (!reportWork)
			return;
		LOG(std::forward<Args>(args)...);
	}
public:
	StablePosAllocator() = default;
	StablePosAllocator(size_t initialSize) :StablePosAllocator() {
		data.reserve(initialSize);
	}
	virtual void Reset() {
		CompactGaps();
		//upon compaction all allocations should go away
		assert(data.empty());
		assert(sizeToPositions.empty());
		assert(positionToSize.empty());
	}

	virtual size_t Allocate(size_t numElems);
	virtual void Free(size_t firstElem, size_t numElems, const T* T0 = nullptr);
	const size_t GetSize() const { return data.size(); }
	const std::vector<T>& GetData() const { return data; }
	      std::vector<T>& GetData()       { return data; }

	virtual const T& operator[](std::size_t idx) const { return data[idx]; }
	virtual       T& operator[](std::size_t idx)       { return data[idx]; }

	static constexpr std::size_t INVALID_INDEX = ~0u;
private:
	void CompactGaps();
private:
	std::vector<T> data;
	std::multimap<size_t, size_t> sizeToPositions;
	std::map<size_t, size_t> positionToSize;
};

template<typename T>
inline size_t StablePosAllocator<T>::Allocate(size_t numElems)
{
	if (numElems == 0)
		return ~0u;

	//no gaps
	if (positionToSize.empty()) {
		size_t returnPos = data.size();
		data.resize(data.size() + numElems);
		myLog("StablePosAllocator<T>::Allocate(%u) = %u [thread_id = %u]", uint32_t(numElems), uint32_t(returnPos), static_cast<uint32_t>(Threading::GetCurrentThreadId()));
		return returnPos;
	}

	//try to find gaps >= in size than requested
	for (auto it = sizeToPositions.lower_bound(numElems); it != sizeToPositions.end(); ++it) {
		if (it->first < numElems)
			continue;

		size_t returnPos = it->second;
		positionToSize.erase(it->second);

		if (it->first > numElems) {
			size_t gapSize = it->first - numElems;
			size_t gapPos = it->second + numElems;
			sizeToPositions.emplace(gapSize, gapPos);
			positionToSize.emplace(gapPos, gapSize);
		}

		sizeToPositions.erase(it);
		myLog("StablePosAllocator<T>::Allocate(%u) = %u", uint32_t(numElems), uint32_t(returnPos));
		return returnPos;
	}

	//all gaps are too small
	size_t returnPos = data.size();
	data.resize(data.size() + numElems);
	myLog("StablePosAllocator<T>::Allocate(%u) = %u", uint32_t(numElems), uint32_t(returnPos));
	return returnPos;
}

//merge adjacent gaps and trim data vec
template<typename T>
inline void StablePosAllocator<T>::CompactGaps()
{
	if (positionToSize.empty())
		return;

	//helper to erase {size, pos} pair from sizeToPositions multimap
	const auto eraseSizeToPositionsKVFunc = [this](size_t size, size_t pos) {
		auto [beg, end] = sizeToPositions.equal_range(size);
		for (auto it = beg; it != end; /*noop*/)
			if (it->second == pos) {
				it = sizeToPositions.erase(it);
				break;
			}
			else {
				++it;
			}
	};

	bool found;
	std::size_t posStartFrom = 0u;
	do {
		found = false;

		std::map<size_t, size_t>::iterator posSizeBeg = positionToSize.lower_bound(posStartFrom);
		std::map<size_t, size_t>::iterator posSizeFin = positionToSize.end(); std::advance(posSizeFin, -1);

		for (auto posSizeThis = posSizeBeg; posSizeThis != posSizeFin; ++posSizeThis) {
			posStartFrom = posSizeThis->first;
			auto posSizeNext = posSizeThis; std::advance(posSizeNext, 1);

			if (posSizeThis->first + posSizeThis->second == posSizeNext->first) {
				std::size_t newPos = posSizeThis->first;
				std::size_t newSize = posSizeThis->second + posSizeNext->second;

				eraseSizeToPositionsKVFunc(posSizeThis->second, posSizeThis->first);
				eraseSizeToPositionsKVFunc(posSizeNext->second, posSizeNext->first);

				positionToSize.erase(posSizeThis);
				positionToSize.erase(posSizeNext); //this iterator is guaranteed to stay valid after 1st erase

				positionToSize.emplace(newPos, newSize);
				sizeToPositions.emplace(newSize, newPos);

				found = true;

				break;
			}
		}
	} while (found);

	std::map<size_t, size_t>::iterator posSizeFin = positionToSize.end(); std::advance(posSizeFin, -1);
	if (posSizeFin->first + posSizeFin->second == data.size()) {
		//trim data vector
		data.resize(posSizeFin->first);
		//erase old sizeToPositions
		eraseSizeToPositionsKVFunc(posSizeFin->second, posSizeFin->first);
		//erase old positionToSize
		positionToSize.erase(posSizeFin);
	}
}

template<typename T>
inline void StablePosAllocator<T>::Free(size_t firstElem, size_t numElems, const T* T0)
{
	assert(firstElem + numElems <= data.size());

	if (numElems == 0) {
		myLog("StablePosAllocator<T>::Free(%u, %u)", uint32_t(firstElem), uint32_t(numElems));
		return;
	}

	if (T0)
		std::fill(data.begin() + firstElem, data.begin() + firstElem + numElems, *T0);

	//lucky us, just remove trim the vector size
	if (firstElem + numElems == data.size()) {
		myLog("StablePosAllocator<T>::Free(%u, %u)", uint32_t(firstElem), uint32_t(numElems));
		data.resize(firstElem);
		return;
	}

	positionToSize.emplace(firstElem, numElems);
	sizeToPositions.emplace(numElems, firstElem);

	static constexpr float compactionTriggerFraction = 0.025f;
	if (positionToSize.size() >= std::ceil(compactionTriggerFraction * data.size()))
		CompactGaps();

	myLog("StablePosAllocator<T>::Free(%u, %u)", uint32_t(firstElem), uint32_t(numElems));
}

#endif

