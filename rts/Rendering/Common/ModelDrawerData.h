#pragma once

#include <vector>
#include <array>
#include <functional>

#include <unordered_map>

#include "System/EventClient.h"
#include "System/EventHandler.h"
#include "System/ContainerUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/Threading/ThreadPool.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Models/ModelsMemStorage.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Env/IWater.h"
#include "Map/ReadMap.h"
#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Game/CameraHandler.h"

class CModelDrawerDataConcept : public CEventClient {
public:
	CModelDrawerDataConcept(const std::string& ecName, int ecOrder)
		: CEventClient(ecName, ecOrder, false)
	{};
	virtual ~CModelDrawerDataConcept() {
		eventHandler.RemoveClient(this);
		autoLinkedEvents.clear();
	};
public:
	bool GetFullRead() const override { return true; }
	int  GetReadAllyTeam() const override { return AllAccessTeam; }
protected:
	static constexpr int MT_CHUNK_OR_MIN_CHUNK_SIZE_SMMA = 128;
	static constexpr int MT_CHUNK_OR_MIN_CHUNK_SIZE_UPDT = 256;
};


template <typename T>
class CModelDrawerDataBase : public CModelDrawerDataConcept
{
public:
	using ObjType = T;
public:
	CModelDrawerDataBase(const std::string& ecName, int ecOrder, bool& mtModelDrawer_);
	virtual ~CModelDrawerDataBase() override;
public:
	virtual void Update() = 0;
protected:
	virtual bool IsAlpha(const T* co) const = 0;
private:
	void AddObject(const T* co, bool add); //never to be called directly! Use UpdateObject() instead!
protected:
	void DelObject(const T* co, bool del);
	void UpdateObject(const T* co, bool init);
protected:
	void UpdateCommon(T* o);
	virtual void UpdateObjectDrawFlags(CSolidObject* o) const = 0;
private:
	void UpdateObjectTrasform(const T* o);
	void UpdateObjectUniforms(const T* o);
public:
	const std::vector<T*>& GetUnsortedObjects() const { return unsortedObjects; }
	const ModelRenderContainer<T>& GetModelRenderer(int modelType) const { return modelRenderers[modelType]; }

	void ClearPreviousDrawFlags() { for (auto object : unsortedObjects) object->previousDrawFlag = 0; }

	const auto& GetObjectTransformMemAlloc(const T* o) const {
		const auto it = scTransMemAllocMap.find(const_cast<T*>(o));
		return (it != scTransMemAllocMap.end()) ? it->second : ScopedTransformMemAlloc::Dummy();
	}
	auto& GetObjectTransformMemAlloc(const T* o) { return scTransMemAllocMap[const_cast<T*>(o)]; }
private:
	static constexpr int MMA_SIZE0 = 2 << 17;
protected:
	std::array<ModelRenderContainer<T>, MODELTYPE_CNT> modelRenderers;

	std::vector<T*> unsortedObjects;
	std::unordered_map<T*, ScopedTransformMemAlloc> scTransMemAllocMap;
	std::unordered_map<const T*, int32_t> lastSyncedFrameUpload;

	bool& mtModelDrawer;
};

using CUnitDrawerDataBase = CModelDrawerDataBase<CUnit>;
using CFeatureDrawerDataBase = CModelDrawerDataBase<CFeature>;

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline CModelDrawerDataBase<T>::CModelDrawerDataBase(const std::string& ecName, int ecOrder, bool& mtModelDrawer_)
	: CModelDrawerDataConcept(ecName, ecOrder)
	, mtModelDrawer(mtModelDrawer_)
{
	scTransMemAllocMap.reserve(MMA_SIZE0);
	for (auto& mr : modelRenderers) { mr.Clear(); }
}

template<typename T>
inline CModelDrawerDataBase<T>::~CModelDrawerDataBase()
{
	unsortedObjects.clear();
	scTransMemAllocMap.clear();
}

template<typename T>
inline void CModelDrawerDataBase<T>::AddObject(const T* co, bool add)
{
	T* o = const_cast<T*>(co);

	if (o->model != nullptr) {
		modelRenderers[MDL_TYPE(o)].AddObject(o);
	}

	if (!add)
		return;

	unsortedObjects.emplace_back(o);

	const uint32_t numMatrices = ((o->model ? o->model->numPieces : 0) + 1u) * 2;
	scTransMemAllocMap.emplace(o, ScopedTransformMemAlloc(numMatrices));
	lastSyncedFrameUpload.emplace(o, -1);

	modelUniformsStorage.AddObject(co);
}

template<typename T>
inline void CModelDrawerDataBase<T>::DelObject(const T* co, bool del)
{
	T* o = const_cast<T*>(co);

	if (o->model != nullptr) {
		modelRenderers[MDL_TYPE(o)].DelObject(o);
	}

	if (del && spring::VectorErase(unsortedObjects, o)) {
		scTransMemAllocMap.erase(o);
		lastSyncedFrameUpload.erase(o);
		modelUniformsStorage.DelObject(co);
	}
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateObject(const T* co, bool init)
{
	DelObject(co, false);
	AddObject(co, init );
}


template<typename T>
inline void CModelDrawerDataBase<T>::UpdateObjectTrasform(const T* o)
{
	// check if already uploaded
	auto lastUploadFrameIt = lastSyncedFrameUpload.find(o);
	assert(lastUploadFrameIt != lastSyncedFrameUpload.end());
	if (lastUploadFrameIt->second >= gs->frameNum)
		return;

	ScopedTransformMemAlloc& stma = GetObjectTransformMemAlloc(o);

	const auto& tmPrev = o->preFrameTra;
	const auto  tmCurr = Transform::FromMatrix(o->GetTransformMatrix(true)); //synced transform

	// conditionally update new and prev synced positions
	stma.UpdateIfChanged(0, tmPrev);
	stma.UpdateIfChanged(1, tmCurr);

	for (int i = 0; i < o->localModel.pieces.size(); ++i) {
		const LocalModelPiece& lmp = o->localModel.pieces[i];

		const auto& lmpTransform = lmp.GetModelSpaceTransform(); //forces dirty / wasUpdated recalculation if no other method called it yet

		if likely(!lmp.GetWasUpdated())
			continue;

		if unlikely(!lmp.GetScriptVisible()) {
			stma.UpdateForced(2 * (1 + i) + 0, Transform::Zero());
			stma.UpdateForced(2 * (1 + i) + 1, Transform::Zero());
			lmp.ResetWasUpdated();
			continue;
		}

		stma.UpdateForced(2 * (1 + i) + 0, lmp.GetPrevModelSpaceTransform());
		stma.UpdateForced(2 * (1 + i) + 1, lmpTransform);

		lmp.ResetWasUpdated();
	}

	lastUploadFrameIt->second = gs->frameNum;
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateObjectUniforms(const T* o)
{
	auto& uni = modelUniformsStorage.GetObjUniformsArray(o);
	uni.drawFlag = o->drawFlag;

	if (gu->spectatingFullView || o->IsInLosForAllyTeam(gu->myAllyTeam)) {
		uni.id = o->id;
		// TODO remove drawPos, replace with pos
		uni.drawPos = float4{ o->drawPos, o->heading * math::PI / SPRING_MAX_HEADING };
		uni.speed = o->speed;
		uni.maxHealth = o->maxHealth;
		uni.health = o->health;
	}
}

template<typename T>
inline void CModelDrawerDataBase<T>::UpdateCommon(T* o)
{
	assert(o);
	o->previousDrawFlag = o->drawFlag;
	UpdateObjectDrawFlags(o);

	if (o->alwaysUpdateMat || (o->drawFlag > DrawFlags::SO_NODRAW_FLAG && o->drawFlag < DrawFlags::SO_DRICON_FLAG))
		UpdateObjectTrasform(o);

	UpdateObjectUniforms(o);
}