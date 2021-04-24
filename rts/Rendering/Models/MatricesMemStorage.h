#pragma once

#include <memory>

#include "System/Matrix44f.h"
#include "System/SpringMem.h"

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
	std::unique_ptr<spring::StablePosAllocator<CMatrix44f>> spa;
};

#define matricesMemStorage MatricesMemStorage::GetInstance()