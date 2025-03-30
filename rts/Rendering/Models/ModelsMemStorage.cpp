#include "ModelsMemStorage.h"
#include "Sim/Objects/WorldObject.h"

#include "System/Misc/TracyDefs.h"

TransformsMemStorage transformsMemStorage;
ModelUniformsStorage modelUniformsStorage;

////////////////////////////////////////////////////////////////////

void ModelUniformsStorage::Init()
{
	assert(updateList.Empty());
	assert(objectsMap.empty());
	assert(storage.empty());

	storage[AddObject(static_cast<const CWorldObject*>(nullptr))] = dummy;
}

void ModelUniformsStorage::Kill()
{
	// Remaining objects are not cleared anywhere (not a good thing) so delete them here
	updateList.Clear();
	storage.clear();
	objectsMap.clear();
}

size_t ModelUniformsStorage::AddObject(const CWorldObject* o)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const size_t idx = storage.Add(ModelUniformData());
	objectsMap[const_cast<CWorldObject*>(o)] = idx;

	if (idx + 1 == storage.size()) {
		//new item got added to the end of storage
		updateList.EmplaceBackUpdate();
	} else {
		// storage got updated somewhere in the middle, use updateList.SetUpdate()
		updateList.SetUpdate(idx);
	}

	return idx;
}

void ModelUniformsStorage::DelObject(const CWorldObject* o)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = objectsMap.find(const_cast<CWorldObject*>(o));

	if (it == objectsMap.end())
		return;

	storage.Del(it->second);

	if (storage.size() < updateList.Size()) {
		// storage got one element shorter, trim updateList as well
		updateList.Trim(it->second);
	} else {
		// storage got updated somewhere in the middle, use updateList.SetUpdate()
		updateList.SetUpdate(it->second);
	}

	objectsMap.erase(it);
}

size_t ModelUniformsStorage::GetObjOffset(const CWorldObject* o)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = objectsMap.find(const_cast<CWorldObject*>(o));
	if (it != objectsMap.end())
		return it->second;

	size_t idx = AddObject(o);
	return idx;
}

size_t ModelUniformsStorage::GetObjOffset(const CWorldObject* o) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = objectsMap.find(const_cast<CWorldObject*>(o));
	if (it != objectsMap.end())
		return it->second;

	return INVALID_INDEX;
}

const ModelUniformsStorage::MyType& ModelUniformsStorage::GetObjUniformsArray(const CWorldObject* o) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	size_t offset = GetObjOffset(o);
	return storage[offset];
}

ModelUniformsStorage::MyType& ModelUniformsStorage::GetObjUniformsArray(const CWorldObject* o)
{
	RECOIL_DETAILED_TRACY_ZONE;
	size_t offset = GetObjOffset(o);
	updateList.SetUpdate(offset);
	return storage[offset];
}

////////////////////////////////////////////////////////////////////

TransformsMemStorage::TransformsMemStorage()
	: storage(StablePosAllocator<MyType>(INIT_NUM_ELEMS))
	, updateList(INIT_NUM_ELEMS)
{}

void TransformsMemStorage::Reset()
{
	assert(Threading::IsMainThread());
	storage.Reset();
	updateList.Clear();
}

size_t TransformsMemStorage::Allocate(size_t numElems)
{
	auto lock = CModelsLock::GetScopedLock();

	auto res = storage.Allocate(numElems);
	updateList.Resize(storage.GetSize());

	return res;
}

void TransformsMemStorage::Free(size_t firstElem, size_t numElems, const MyType* T0)
{
	auto lock = CModelsLock::GetScopedLock();

	storage.Free(firstElem, numElems, T0);
	updateList.SetUpdate(firstElem, numElems);
	updateList.Trim(storage.GetSize());
}

const TransformsMemStorage::MyType& TransformsMemStorage::operator[](std::size_t idx) const
{
	auto lock = CModelsLock::GetScopedLock();

	return storage[idx];
}