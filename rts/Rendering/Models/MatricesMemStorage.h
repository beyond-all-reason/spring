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
	static constexpr size_t INVALID_INDEX = ~0u;
private:
	std::unique_ptr<StablePosAllocator<CMatrix44f>> spa;
};

#define matricesMemStorage MatricesMemStorage::GetInstance()


////////////////////////////////////////////////////////////////////

class MatAllocElem {
public:
	MatAllocElem() { Reset(); };
	MatAllocElem(const MatAllocElem&) = delete;
	MatAllocElem(MatAllocElem&& wmma) noexcept { *this = std::move(wmma); }
public:
	friend class ScopedMatricesMemAlloc;
private:
	MatAllocElem(size_t elem_)
		: elem{ elem_ }
	{ }
	void Reset() {
		elem = MatricesMemStorage::INVALID_INDEX;
	}
public:
	MatAllocElem& operator= (const MatAllocElem&) = delete;
	MatAllocElem& operator= (MatAllocElem&& mae) noexcept {
		std::swap(elem, mae.elem);

		return *this;
	}
	const CMatrix44f& operator()() const {
		assert(elem != MatricesMemStorage::INVALID_INDEX);
		return matricesMemStorage[elem];
	}
	CMatrix44f& operator()() {
		assert(elem != MatricesMemStorage::INVALID_INDEX);
		return matricesMemStorage[elem];
	}
private:
	size_t elem = MatricesMemStorage::INVALID_INDEX;
};

class ScopedMatricesMemAlloc {
public:
	ScopedMatricesMemAlloc() : ScopedMatricesMemAlloc(0u) {};
	ScopedMatricesMemAlloc(size_t numElems_, bool withMutex = false) : numElems{numElems_} {
		firstElem = matricesMemStorage.Allocate(numElems, withMutex);

		for (size_t i = 0; i < numElems; ++i)
			matAllocElem.emplace_back(MatAllocElem(firstElem + i));
	}

	ScopedMatricesMemAlloc(const ScopedMatricesMemAlloc&) = delete;
	ScopedMatricesMemAlloc(ScopedMatricesMemAlloc&& smma) noexcept { *this = std::move(smma); }


	~ScopedMatricesMemAlloc() {
		if (firstElem == MatricesMemStorage::INVALID_INDEX)
			return;

		for (auto& mae : matAllocElem)
			mae.Reset();

		matricesMemStorage.Free(firstElem, numElems);
	}

	ScopedMatricesMemAlloc& operator= (const ScopedMatricesMemAlloc&) = delete;
	ScopedMatricesMemAlloc& operator= (ScopedMatricesMemAlloc&& smma) noexcept {
		//swap to prevent dealloc on dying object, yet enable destructor to do its thing on valid object
		std::swap(firstElem, smma.firstElem);
		std::swap(numElems , smma.numElems );
		std::swap(matAllocElem, smma.matAllocElem);

		return *this;
	}

	const MatAllocElem& operator[](size_t offset) const {
		assert(offset >= 0 && offset < numElems);
		return matAllocElem[offset];
	}
	MatAllocElem& operator[](size_t offset) {
		assert(offset >= 0 && offset < numElems);
		return matAllocElem[offset];
	}
private:
	std::vector<MatAllocElem> matAllocElem;
	size_t firstElem = MatricesMemStorage::INVALID_INDEX;
	size_t numElems  = 0u;
};
