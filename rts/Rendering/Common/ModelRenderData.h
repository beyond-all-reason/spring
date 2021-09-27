#pragma once

#include <vector>
#include <array>

#include <unordered_map>

#include "System/EventClient.h"
#include "System/EventHandler.h"
#include "System/ContainerUtil.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Models/MatricesMemStorage.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Rendering/Models/3DModel.h"
#include "Map/ReadMap.h"
#include "Game/Camera.h"

class CModelRenderDataConcept : public CEventClient {
public:
	CModelRenderDataConcept(const std::string& ecName, int ecOrder)
		: CEventClient(ecName, ecOrder, false)
		, drawQuadsX{ mapDims.mapx / CModelRenderDataConcept::DRAW_QUAD_SIZE }
		, drawQuadsY{ mapDims.mapy / CModelRenderDataConcept::DRAW_QUAD_SIZE }
	{};
	virtual ~CModelRenderDataConcept() {
		eventHandler.RemoveClient(this);
		autoLinkedEvents.clear();
	};
public:
	bool GetFullRead() const override { return true; }
	int  GetReadAllyTeam() const override { return AllAccessTeam; }
public:
	int drawQuadsX;
	int drawQuadsY;
protected:
	static constexpr int DRAW_QUAD_SIZE = 32;
};


template <typename T>
class CModelRenderDataBase : public CModelRenderDataConcept
{
public:
	CModelRenderDataBase(const std::string& ecName, int ecOrder);
	virtual ~CModelRenderDataBase() override;
public:
	virtual void Update() = 0;
protected:
	virtual bool IsAlpha(const T* co) const = 0;
protected:
	void DelObject(const T* co);
private:
	void AddObject(const T* co); //never to be called directly! Use UpdateDrawQuad() instead!
protected:
	void UpdateDrawQuad(const T* co);
public:
	void UpdateVisibleQuads(CCamera* cam, float maxDist, int xzExtraSize = 0, float extraHeight = 100.0f);
public:
	class CModelQuadDrawer; //fwd declaration for friend class to work under GCC

	class RdrContProxy {
	public:
		// mrc proxy functions
		void Init() { mrc.Init(); lastDrawFrame = 0; }
		void Kill() { mrc.Kill(); }

		void AddObject(const T* co) { mrc.AddObject(co); }
		void DelObject(const T* co) { mrc.DelObject(co); }

		bool HasObjects() const { return mrc.GetNumObjects() > 0; }

		uint32_t GetNumObjects() const { return mrc.GetNumObjects(); }
		uint32_t GetNumObjectBins() const { return mrc.GetNumObjectBins(); }
		uint32_t GetObjectBinKey(uint32_t idx) const { return mrc.GetObjectBinKey(idx); }
		const auto& GetObjectBin(uint32_t idx) const { return mrc.GetObjectBin(idx); }

		//uint32_t GetLastDrawFrame() const { return lastDrawFrame; }

		bool IsQuadVisible() const { return lastDrawFrame >= globalRendering->drawFrame; }
	private:
		friend class CModelQuadDrawer;
		ModelRenderContainer<T> mrc;
		// frame on which this proxy's owner quad last
		// received a DrawQuad call (i.e. was in view)
		// during *any* pass
		uint32_t lastDrawFrame = 0;
	};

	// [modelType][quad] ==> RdrContProxy --> ModelRenderContainer<T>
	using ModelRenderers = std::array<std::vector<RdrContProxy>, MODELTYPE_CNT>;
	using Quads = std::vector<int>;

	class CModelQuadDrawer : public CReadMap::IQuadDrawer {
	public:
		CModelQuadDrawer() : numQuadsX(0) {}
		CModelQuadDrawer(int _numQuadsX)
			: numQuadsX(_numQuadsX)
		{ }

		void ResetState() override {
			camQuads.clear();
			for (auto& rdrProxy : rdrProxies)
				rdrProxy.clear();
		}

		void DrawQuad(int x, int y) override {
			const int index = y * numQuadsX + x;
			camQuads.emplace_back(index);

			// used so we do not iterate over non-visited renderers (in any pass)
			for (auto& rdrProxy : rdrProxies)
				rdrProxy[index].lastDrawFrame = globalRendering->drawFrame;
		}
	public:
		Quads& GetCamQuads() { return camQuads; }
		ModelRenderers& GetRdrProxies() { return rdrProxies; }

	private:
		Quads camQuads;
		ModelRenderers rdrProxies;

		int numQuadsX;
	};
public:
	const std::vector<RdrContProxy>& GetRdrContProxies(int modelType) const { return modelRenderers[modelType]; }
	const Quads& GetCamVisibleQuads(int camType) const { return camVisibleQuads[camType]; }
	uint32_t GetCamVisDrawFrames(int camType) const { return camVisDrawFrames[camType]; }

	const std::vector<T*>& GetUnsortedObjects() const { return unsortedObjects; }

	const ScopedMatricesMemAlloc& GetObjectMatricesMemAlloc(const T* o) const {
		const auto it = matricesMemAllocs.find(const_cast<T*>(o));
		return (it != matricesMemAllocs.end()) ? it->second : ScopedMatricesMemAlloc::Dummy();
	}
	ScopedMatricesMemAlloc& GetObjectMatricesMemAlloc(const T* o) { return matricesMemAllocs[const_cast<T*>(o)]; }
private:
	static constexpr int MMA_SIZE0 = 2 << 16;
protected:
	ModelRenderers modelRenderers;

	std::array<Quads, CCamera::CAMTYPE_ENVMAP> camVisibleQuads;
	std::array<uint32_t, CCamera::CAMTYPE_ENVMAP> camVisDrawFrames;

	std::vector<T*> unsortedObjects;
	std::unordered_map<T*, ScopedMatricesMemAlloc> matricesMemAllocs;

	CModelQuadDrawer quadDrawer;
};

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
inline CModelRenderDataBase<T>::CModelRenderDataBase(const std::string& ecName, int ecOrder)
	: CModelRenderDataConcept(ecName, ecOrder)
{
	quadDrawer = CModelQuadDrawer(drawQuadsX);

	camVisDrawFrames.fill(0);
	matricesMemAllocs.reserve(MMA_SIZE0);

	for (auto& mr : modelRenderers) {
		mr.resize(drawQuadsX * drawQuadsY);
		for (auto& rp : mr) {
			rp.Init();
		}
	}

	for (auto& camVisibleQuad : camVisibleQuads) {
		camVisibleQuad.clear();
		camVisibleQuad.reserve(drawQuadsX * drawQuadsY);
	}
}

template<typename T>
inline CModelRenderDataBase<T>::~CModelRenderDataBase()
{
	for (auto& mr : modelRenderers) {
		for (auto& rp : mr) {
			rp.Kill();
		}
		mr.clear();
	}

	unsortedObjects.clear();
	matricesMemAllocs.clear();
}

template<typename T>
inline void CModelRenderDataBase<T>::AddObject(const T* co)
{
	T* o = const_cast<T*>(co);

	if (o->model != nullptr) {
		modelRenderers[MDL_TYPE(o)][o->drawQuad].AddObject(o);
	}

	unsortedObjects.emplace_back(o);

	const uint32_t numMatrices = (o->model ? o->model->numPieces : 0) + 1u;
	matricesMemAllocs.emplace(o, ScopedMatricesMemAlloc(numMatrices));
}

template<typename T>
inline void CModelRenderDataBase<T>::DelObject(const T* co)
{
	T* o = const_cast<T*>(co);

	if (o->model != nullptr && o->drawQuad >= 0)
		modelRenderers[MDL_TYPE(o)][o->drawQuad].DelObject(o);

	spring::VectorErase(unsortedObjects, o);
	matricesMemAllocs.erase(o);
}

template<typename T>
inline void CModelRenderDataBase<T>::UpdateDrawQuad(const T* co)
{
	const int newDrawQuadX = std::clamp(int(co->pos.x / SQUARE_SIZE / CModelRenderDataConcept::DRAW_QUAD_SIZE), 0, drawQuadsX - 1);
	const int newDrawQuadY = std::clamp(int(co->pos.z / SQUARE_SIZE / CModelRenderDataConcept::DRAW_QUAD_SIZE), 0, drawQuadsY - 1);
	const int newDrawQuad = newDrawQuadY * drawQuadsX + newDrawQuadX;

	if (co->drawQuad == newDrawQuad)
		return;

	//TODO check if out of map objects get drawn, when the camera is outside of the map
	assert(co->drawQuad < drawQuadsX * drawQuadsY);
	assert(newDrawQuad < drawQuadsX * drawQuadsY && newDrawQuad >= 0);

	DelObject(co);
	//update draw quad, mandatory for AddObject() to work correctly
	(const_cast<T*>(co))->drawQuad = newDrawQuad;
	AddObject(co);
}

template<typename T>
inline void CModelRenderDataBase<T>::UpdateVisibleQuads(CCamera* cam, float maxDist, int xzExtraSize, float extraHeight)
{
	const uint32_t camType = cam->GetCamType();

	// already done
	if (camVisDrawFrames[camType] >= globalRendering->drawFrame) {
		return;
	}

	camVisDrawFrames[camType] = globalRendering->drawFrame;

	camVisibleQuads[camType].clear();
	camVisibleQuads[camType].reserve(256);

	{
		quadDrawer.ResetState();
		(quadDrawer.GetCamQuads()).swap(camVisibleQuads[camType]);
		(quadDrawer.GetRdrProxies()).swap(modelRenderers);

		cam->CalcFrustumLines(readMap->GetCurrMinHeight() - extraHeight, readMap->GetCurrMaxHeight() + extraHeight, SQUARE_SIZE);
		readMap->GridVisibility(cam, &quadDrawer, maxDist, CModelRenderDataConcept::DRAW_QUAD_SIZE, xzExtraSize);

		(quadDrawer.GetCamQuads()).swap(camVisibleQuads[camType]);
		(quadDrawer.GetRdrProxies()).swap(modelRenderers);
	}
}

using CUnitRenderDataBase = CModelRenderDataBase<CUnit>;
using CFeatureRenderDataBase = CModelRenderDataBase<CFeature>;