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

class ScopedMatricesMemAlloc;
class MatAllocElem {
public:
	MatAllocElem() : MatAllocElem(MatricesMemStorage::INVALID_INDEX, nullptr) {};
	MatAllocElem(const MatAllocElem& wmma) = default;
	MatAllocElem(MatAllocElem&& wmma) = default;
public:
	friend class ScopedMatricesMemAlloc;
private:
	MatAllocElem(std::size_t elem_, const ScopedMatricesMemAlloc* smma_)
		: elem{ elem_ }
		, smma{ smma_ }
	{ }
public:
	bool Valid() const { return elem != MatricesMemStorage::INVALID_INDEX; }

	MatAllocElem& operator= (const MatAllocElem& mae) = default;
	MatAllocElem& operator= (MatAllocElem&& mae) = default;
	const CMatrix44f& operator()() const {
		assert(elem != MatricesMemStorage::INVALID_INDEX);
		assert(smma != nullptr);
		return matricesMemStorage[elem];
	}
	CMatrix44f& operator()() {
		assert(elem != MatricesMemStorage::INVALID_INDEX);
		assert(smma != nullptr);
		return matricesMemStorage[elem];
	}
private:
	std::size_t elem;
	const ScopedMatricesMemAlloc* smma;
};

class ScopedMatricesMemAlloc {
public:
	ScopedMatricesMemAlloc() : ScopedMatricesMemAlloc(0u) {};
	ScopedMatricesMemAlloc(std::size_t numElems_, bool withMutex = false) : numElems{numElems_} {
		firstElem = matricesMemStorage.Allocate(numElems, withMutex);
	}

	ScopedMatricesMemAlloc(const ScopedMatricesMemAlloc&) = delete;
	ScopedMatricesMemAlloc(ScopedMatricesMemAlloc&& smma) noexcept { *this = std::move(smma); }


	~ScopedMatricesMemAlloc() {
		if (firstElem == MatricesMemStorage::INVALID_INDEX)
			return;

		matricesMemStorage.Free(firstElem, numElems);
	}

	bool Valid() const { return firstElem != MatricesMemStorage::INVALID_INDEX;	}
	std::size_t GetOffset() const { assert(Valid()); return firstElem; }

	ScopedMatricesMemAlloc& operator= (const ScopedMatricesMemAlloc&) = delete;
	ScopedMatricesMemAlloc& operator= (ScopedMatricesMemAlloc&& smma) noexcept {
		//swap to prevent dealloc on dying object, yet enable destructor to do its thing on valid object
		std::swap(firstElem, smma.firstElem);
		std::swap(numElems , smma.numElems );

		return *this;
	}

	const MatAllocElem operator[](std::size_t offset) const {
		assert(offset >= 0 && offset < numElems);
		return MatAllocElem(firstElem + offset, this);
	}
	MatAllocElem operator[](std::size_t offset) {
		assert(offset >= 0 && offset < numElems);
		return MatAllocElem(firstElem + offset, this);
	}
private:
	std::size_t firstElem = MatricesMemStorage::INVALID_INDEX;
	std::size_t numElems  = 0u;
};
