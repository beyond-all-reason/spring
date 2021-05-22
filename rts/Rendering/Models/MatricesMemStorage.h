#pragma once

#include <memory>

#include "System/Matrix44f.h"
#include "System/MemPoolTypes.h"

class MatricesMemStorage {
public:
	static MatricesMemStorage& GetInstance() {
		static MatricesMemStorage instance;
		return instance;
	};
public:
	MatricesMemStorage();
public:
	void Reset() { spa->Reset(); };
	size_t Allocate(size_t numElems, bool forceMutex = false) { return spa->Allocate(numElems, forceMutex); };
	void Free(size_t& firstElem, size_t numElems) { spa->Free(firstElem, numElems); };
    const size_t GetSize() const { return spa->GetSize(); }
    std::vector<CMatrix44f>& GetData() { return spa->GetData(); }

	const CMatrix44f& operator[](std::size_t idx) const { return spa->operator[](idx); }
	      CMatrix44f& operator[](std::size_t idx)       { return spa->operator[](idx); }
public:
	static constexpr int INIT_NUM_ELEMS = 1 << 16u;
	static constexpr size_t INVALID_INDEX = ~0u;
private:
	std::unique_ptr<StablePosAllocator<CMatrix44f>> spa;
};

#define matricesMemStorage MatricesMemStorage::GetInstance()


////////////////////////////////////////////////////////////////////

class WeakMatricesMemAllocElem {
public:
	WeakMatricesMemAllocElem() { Reset(); };
	WeakMatricesMemAllocElem(const WeakMatricesMemAllocElem&) = delete;
	WeakMatricesMemAllocElem(WeakMatricesMemAllocElem&& wmma) noexcept { *this = std::move(wmma); }
public:
	friend class ScopedMatricesMemAlloc;
private:
	WeakMatricesMemAllocElem(size_t elem_)
		: elem{ elem_ }
	{ }
	void Reset() {
		elem = MatricesMemStorage::INVALID_INDEX;
	}
public:
	WeakMatricesMemAllocElem& operator= (const WeakMatricesMemAllocElem&) = delete;
	WeakMatricesMemAllocElem& operator= (WeakMatricesMemAllocElem&&) = default;
	const CMatrix44f& operator()() const {
		assert(elem != MatricesMemStorage::INVALID_INDEX);
		return matricesMemStorage[elem];
	}
	CMatrix44f& operator()() {
		assert(elem != ~0u);
		return matricesMemStorage[elem];
	}
private:
	size_t elem = MatricesMemStorage::INVALID_INDEX;
};

class ScopedMatricesMemAlloc {
public:
	ScopedMatricesMemAlloc() : ScopedMatricesMemAlloc(0u) {};
	ScopedMatricesMemAlloc(size_t numElems_, bool forceMutex = false) : numElems{numElems_} {
		firstElem = matricesMemStorage.Allocate(numElems, forceMutex);

		for (size_t i = 0; i < numElems; ++i)
			weakMatricesMemAllocs.emplace_back(WeakMatricesMemAllocElem(firstElem + i));
	}

	ScopedMatricesMemAlloc(const ScopedMatricesMemAlloc&) = delete;
	ScopedMatricesMemAlloc(ScopedMatricesMemAlloc&& smma) noexcept { *this = std::move(smma); }


	~ScopedMatricesMemAlloc() {
		if (firstElem == MatricesMemStorage::INVALID_INDEX)
			return;

		for (auto& wmma : weakMatricesMemAllocs)
			wmma.Reset();

		matricesMemStorage.Free(firstElem, numElems);
	}

	ScopedMatricesMemAlloc& operator= (const ScopedMatricesMemAlloc&) = delete;
	ScopedMatricesMemAlloc& operator= (ScopedMatricesMemAlloc&& smma) noexcept {
		firstElem = smma.firstElem;
		numElems = smma.numElems;

		weakMatricesMemAllocs = std::move(smma.weakMatricesMemAllocs);

		//prevent dealloc on dying object
		smma.firstElem = MatricesMemStorage::INVALID_INDEX;
		smma.numElems  = 0u;

		return *this;
	}

	const WeakMatricesMemAllocElem& operator[](size_t offset) const {
		assert(offset >= 0 && offset < numElems);
		return weakMatricesMemAllocs[offset];
	}
	WeakMatricesMemAllocElem& operator[](size_t offset) {
		assert(offset >= 0 && offset < numElems);
		return weakMatricesMemAllocs[offset];
	}
private:
	std::vector<WeakMatricesMemAllocElem> weakMatricesMemAllocs;
	size_t firstElem = MatricesMemStorage::INVALID_INDEX;
	size_t numElems  = 0u;
};
