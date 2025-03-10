#pragma once

#include <memory>
#include <vector>

#include "ModelsMemStorageDefs.h"
#include "ModelsLock.h"
#include "System/Transform.hpp"
#include "System/MemPoolTypes.h"
#include "System/FreeListMap.h"
#include "System/UnorderedMap.hpp"
#include "System/TypeToStr.h"
#include "System/Threading/SpringThreading.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "Rendering/Common/UpdateList.h"

class TransformsMemStorage {
public:
	using MyType = Transform;
	using EqualCmpFunctor = bool(*)(const MyType&, const MyType&);
public:
	explicit TransformsMemStorage();
	void Reset();

	size_t Allocate(size_t numElems);

	void Free(size_t firstElem, size_t numElems, const MyType* T0 = nullptr);

	const auto& GetData() const { return storage.GetData(); }
	const auto  GetSize() const { return storage.GetSize(); }

	template<typename MyTypeLike = MyType> // to force universal references
	bool UpdateIfChanged(std::size_t idx, MyTypeLike&& newValue, EqualCmpFunctor eqCmp) {
		//auto lock = CModelsLock::GetScopedLock();

		using DT = StablePosAllocator<MyType>;
		const auto& curValue = const_cast<const DT&>(storage)[idx];
		if (eqCmp(curValue, newValue))
			return false;

		updateList.SetUpdate(idx);
		auto& mutValue = const_cast<DT&>(storage)[idx];
		mutValue = newValue;

		assert(updateList.Size() == storage.GetSize());

		return true;
	}

	template<typename MyTypeLike = MyType> // to force universal references
	void UpdateForced(std::size_t idx, MyTypeLike&& newValue) {
		//auto lock = CModelsLock::GetScopedLock();

		updateList.SetUpdate(idx);
		auto& mutValue = storage[idx];
		mutValue = newValue;

		assert(updateList.Size() == storage.GetSize());
	}

	const MyType& operator[](std::size_t idx) const;

	const auto& GetUpdateList() const { return updateList; }
	      auto& GetUpdateList()       { return updateList; }
private:
	StablePosAllocator<MyType> storage;
	UpdateListMT updateList;
private:
	static constexpr int INIT_NUM_ELEMS = 1 << 16u;
public:
	static constexpr auto INVALID_INDEX = StablePosAllocator<MyType>::INVALID_INDEX;
};

extern TransformsMemStorage transformsMemStorage;


////////////////////////////////////////////////////////////////////

class ScopedTransformMemAlloc {
public:
	ScopedTransformMemAlloc() : ScopedTransformMemAlloc(0u) {};
	ScopedTransformMemAlloc(std::size_t numElems_)
		: numElems{numElems_}
	{
		firstElem = transformsMemStorage.Allocate(numElems);
	}

	ScopedTransformMemAlloc(const ScopedTransformMemAlloc&) = delete;
	ScopedTransformMemAlloc(ScopedTransformMemAlloc&& smma) noexcept { *this = std::move(smma); }

	~ScopedTransformMemAlloc() {
		if (firstElem == TransformsMemStorage::INVALID_INDEX)
			return;

		transformsMemStorage.Free(firstElem, numElems, &Transform::Zero());
	}

	bool Valid() const { return firstElem != TransformsMemStorage::INVALID_INDEX;	}
	std::size_t GetOffset(bool assertInvalid = true) const {
		if (assertInvalid)
			assert(Valid());

		return firstElem;
	}

	ScopedTransformMemAlloc& operator= (const ScopedTransformMemAlloc&) = delete;
	ScopedTransformMemAlloc& operator= (ScopedTransformMemAlloc&& smma) noexcept {
		//swap to prevent dealloc on dying object, yet enable destructor to do its thing on valid object
		std::swap(firstElem, smma.firstElem);
		std::swap(numElems , smma.numElems );

		return *this;
	}

	const auto& operator[](std::size_t offset) const {
		assert(firstElem != TransformsMemStorage::INVALID_INDEX);
		assert(offset >= 0 && offset < numElems);

		return transformsMemStorage[firstElem + offset];
	}

	template<typename MyTypeLike = TransformsMemStorage::MyType> // to force universal references
	bool UpdateIfChanged(std::size_t offset, MyTypeLike&& newValue) {
		static const auto EqCmp = [](const TransformsMemStorage::MyType& lhs, const TransformsMemStorage::MyType& rhs) {
			return lhs.equals(rhs);
		};

		assert(firstElem != TransformsMemStorage::INVALID_INDEX);
		assert(offset >= 0 && offset < numElems);

		return transformsMemStorage.UpdateIfChanged(firstElem + offset, std::forward<MyTypeLike>(newValue), EqCmp);
	}

	template<typename MyTypeLike = TransformsMemStorage::MyType> // to force universal references
	void UpdateForced(std::size_t offset, MyTypeLike&& newValue) {
		assert(firstElem != TransformsMemStorage::INVALID_INDEX);
		assert(offset >= 0 && offset < numElems);

		transformsMemStorage.UpdateForced(firstElem + offset, std::forward<MyTypeLike>(newValue));
	}
public:
	static const ScopedTransformMemAlloc& Dummy() {
		static ScopedTransformMemAlloc dummy;

		return dummy;
	};
private:
	std::size_t firstElem = TransformsMemStorage::INVALID_INDEX;
	std::size_t numElems  = 0u;
};

////////////////////////////////////////////////////////////////////

class CWorldObject;
class ModelUniformsStorage {
private:
	using MyType = ModelUniformData;
public:
	void Init();
	void Kill();
public:
	size_t AddObject(const CWorldObject* o);
	size_t GetObjOffset(const CWorldObject* o);
	MyType& GetObjUniformsArray(const CWorldObject* o);
	void   DelObject(const CWorldObject* o);

	size_t AddObject(const SolidObjectDef* o) { return INVALID_INDEX; }
	size_t GetObjOffset(const SolidObjectDef* o) { return INVALID_INDEX; }
	MyType& GetObjUniformsArray(const SolidObjectDef* o) { return dummy; }
	void   DelObject(const SolidObjectDef* o) {}

	size_t AddObject(const S3DModel* o) { return INVALID_INDEX; }
	size_t GetObjOffset(const S3DModel* o) { return INVALID_INDEX; }
	MyType& GetObjUniformsArray(const S3DModel* o) { return dummy; }
	void   DelObject(const S3DModel* o) {}

	auto GetSize() const { return storage.GetData().size(); }
	const auto& GetData() const { return storage.GetData(); }

	const auto& GetUpdateList() const { return updateList; }
	      auto& GetUpdateList()       { return updateList; }
private:
	UpdateList updateList;
public:
	static constexpr size_t INVALID_INDEX = 0;
private:
	inline static MyType dummy = {};

	spring::unordered_map<CWorldObject*, size_t> objectsMap;
	spring::FreeListMap<MyType> storage;
};

extern ModelUniformsStorage modelUniformsStorage;
