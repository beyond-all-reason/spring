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
	size_t Allocate(size_t numElems, bool withMutex = false) { return spa->Allocate(numElems, withMutex); };
	void Free(size_t& firstElem, size_t numElems) { spa->Free(firstElem, numElems); };
    const size_t GetSize() const { return spa->GetSize(); }
    std::vector<CMatrix44f>& GetData() { return spa->GetData(); }

	const CMatrix44f& operator[](std::size_t idx) const { return spa->operator[](idx); }
	      CMatrix44f& operator[](std::size_t idx)       { return spa->operator[](idx); }
public:
	static constexpr int INIT_NUM_ELEMS = 1 << 16u;
private:
	std::unique_ptr<StablePosAllocator<CMatrix44f>> spa;
};

#define matricesMemStorage MatricesMemStorage::GetInstance()

class ScopedMatricesMemAlloc {
public:
	ScopedMatricesMemAlloc() : ScopedMatricesMemAlloc(1u) {};
	ScopedMatricesMemAlloc(size_t numElems_) : numElems{numElems_} {
		firstElem = MatricesMemStorage::GetInstance().Allocate(numElems);
	}
	~ScopedMatricesMemAlloc() {
		if (firstElem < ~0u && numElems > 0)
			MatricesMemStorage::GetInstance().Free(firstElem, numElems);
	}
	ScopedMatricesMemAlloc& operator= (ScopedMatricesMemAlloc&&) = default;
	ScopedMatricesMemAlloc& operator= (const ScopedMatricesMemAlloc&) = delete;

	void operator()(const CMatrix44f& in, size_t offset = 0u) {
		MatricesMemStorage::GetInstance()[firstElem + offset] = in;
	}
	const size_t operator()() const {
		return firstElem;
	}
	const CMatrix44f& operator[](size_t offset) const {
		return MatricesMemStorage::GetInstance()[firstElem + offset];
	}
private:
	size_t firstElem = ~0u;
	size_t numElems  =  0u;
};