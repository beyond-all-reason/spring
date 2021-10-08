#pragma once

#include <array>
#include <string>
#include <string_view>
#include <stack>
#include <tuple>

#include "ModelRenderData.h"
#include "ModelDrawerState.hpp"
#include "System/Log/ILog.h"
#include "System/TypeToStr.h"
#include "Rendering/GL/LightHandler.h"

namespace GL { struct GeometryBuffer; }
template<typename T> class ScopedDrawerImpl;

using DummY = void;

enum ModelDrawerTypes {
	MODEL_DRAWER_FFP =  0, // fixed-function path
	MODEL_DRAWER_ARB =  1, // standard-shader path (ARB)
	MODEL_DRAWER_GLSL = 2, // standard-shader path (GLSL)
	MODEL_DRAWER_GL4 =  3, // GL4-shader path (GLSL)
	MODEL_DRAWER_CNT =  4
};

static constexpr std::string_view ModelDrawerNames[ModelDrawerTypes::MODEL_DRAWER_CNT] = {
	"FFP : fixed-function path",
	"ARB : legacy standard shader path",
	"GLSL: legacy standard shader path",
	"GL4 : modern standard shader path",
};

class CModelDrawerConcept {
public:
	CModelDrawerConcept() {}
	virtual ~CModelDrawerConcept() {}
public:
	static void InitStatic();
	static void KillStatic(bool reload);
public:
	static bool  UseAdvShading() { return advShading; }
	static bool  DeferredAllowed() { return deferredAllowed; }
	static bool& WireFrameModeRef() { return wireFrameMode; }
public:
	// lightHandler
	static GL::LightHandler* GetLightHandler() { return &lightHandler; }

	// geomBuffer
	static GL::GeometryBuffer* GetGeometryBuffer() { return geomBuffer; }
protected:
	inline static bool initialized = false;

	inline static bool advShading = false;
	inline static bool wireFrameMode = false;

	inline static bool deferredAllowed = false;
protected:
	inline static GL::LightHandler lightHandler;
	inline static GL::GeometryBuffer* geomBuffer = nullptr;
};

template <typename TDrawerData, typename TDrawer>
class CModelDrawerBase : public CModelDrawerConcept {
public:
	template<typename TDrawerDerivative>
	static void InitInstance(int t) {
		static_assert(std::is_base_of_v<TDrawer, TDrawerDerivative>, "");
		static_assert(std::is_base_of_v<CModelDrawerBase<TDrawerData, TDrawer>, TDrawer>, "");

		if (modelDrawers[t] == nullptr) {
			modelDrawers[t] = new TDrawerDerivative{};
			modelDrawers[t]->mdType = static_cast<ModelDrawerTypes>(t);
		}
	}
	static void KillInstance(int t) {
		spring::SafeDelete(modelDrawers[t]);
	}

	static void InitStatic();
	static void KillStatic(bool reload);
	static void UpdateStatic() {
		SelectImplementation();
		modelDrawer->Update();
	}
public:
	// Set/Get state from outside
	static bool& UseAdvShadingRef() { reselectionRequested = true; return advShading; }
	static bool& WireFrameModeRef() { return wireFrameMode; }

	static int  PreferedDrawerType() { return preferedDrawerType; }
	static int& PreferedDrawerTypeRef() { reselectionRequested = true; return preferedDrawerType; }

	static bool& MTDrawerTypeRef() { return mtModelDrawer; } //no reselectionRequested needed

	static void SetDrawForwardPass(bool b) { drawForward = b; }
	static void SetDrawDeferredPass(bool b) { drawDeferred = b; }
	static bool DrawForward() { return drawForward; }
	static bool DrawDeferred() { return drawDeferred; }

	static void ForceLegacyPath();

	static void SelectImplementation(bool forceReselection = false, bool legacy = true, bool modern = true);
	static void SelectImplementation(int targetImplementation);

	template<typename T> friend class ScopedDrawerImpl;

	/// Proxy interface for modelDrawerState
	static bool CanDrawDeferred() { return modelDrawerState->CanDrawDeferred(); }
	static bool SetTeamColor(int team, const float2 alpha = float2{ 1.0f, 0.0f }) { return modelDrawerState->SetTeamColor(team, alpha); }
	static void SetNanoColor(const float4& color) { modelDrawerState->SetNanoColor(color); }
public:
	virtual void Update() const = 0;
	// Draw*
	virtual void Draw(bool drawReflection, bool drawRefraction = false) const = 0;

	virtual void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawShadowPass() const = 0;
	virtual void DrawAlphaPass() const = 0;
public:
	// Variuous auxilary drawers call unitDrawer->Setup/Reset...
	// Setup Fixed State
	virtual void SetupOpaqueDrawing(bool deferredPass) const = 0;
	virtual void ResetOpaqueDrawing(bool deferredPass) const = 0;

	virtual void SetupAlphaDrawing(bool deferredPass) const = 0;
	virtual void ResetAlphaDrawing(bool deferredPass) const = 0;
private:
	static void Push(bool legacy, bool modern) {
		implStack.emplace(std::make_pair(modelDrawer, modelDrawerState));
		SelectImplementation(true, legacy, modern);
	}
	static void Pop() {
		std::pair<TDrawer*, IModelDrawerState*> p = implStack.top();  implStack.pop();
		modelDrawer = p.first;
		modelDrawerState = p.second;
	}
private:
	ModelDrawerTypes mdType = ModelDrawerTypes::MODEL_DRAWER_CNT;
public:
	inline static TDrawer* modelDrawer = nullptr;
protected:
	inline static int preferedDrawerType = ModelDrawerTypes::MODEL_DRAWER_CNT; //no preference
	inline static bool mtModelDrawer = true;

	inline static bool reselectionRequested = true;

	inline static bool forceLegacyPath = false;

	inline static bool drawForward = true;
	inline static bool drawDeferred = true;

	inline static TDrawerData* modelDrawerData = nullptr;
	inline static IModelDrawerState* modelDrawerState = nullptr;
	inline static std::array<TDrawer*, ModelDrawerTypes::MODEL_DRAWER_CNT> modelDrawers = {};

	inline static std::stack<std::pair<TDrawer*, IModelDrawerState*>> implStack;
protected:
	static constexpr std::string_view className = spring::TypeToStr<TDrawer>();
};

template <typename TDrawerData, typename TDrawer, bool legacy>
class CModelDrawerCommon : public CModelDrawerBase<TDrawerData, TDrawer> {
private:
	static void UpdateImpl() {
		modelDrawerData->Update();
	}

	template<LuaObjType LOT>
	static void DrawImpl(bool drawReflection, bool drawRefraction);

	template<LuaObjType LOT>
	static void DrawShadowPassImpl();

	static void SetupOpaqueDrawingImpl(bool deferredPass);
	static void ResetOpaqueDrawingImpl(bool deferredPass);
	static void SetupAlphaDrawingImpl(bool deferredPass);
	static void ResetAlphaDrawingImpl(bool deferredPass);
/*
	template<typename T, bool legacy>
	static DrawObjectModelImpl(const T* o, bool noLuaCall);

	template<typename T, bool legacy>
	static DrawObjectTransImpl(const T* o, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall);

	template<typename T, bool legacy>
	static DrawObjectIndividualImpl(const T* o, bool noLuaCall);

	template<typename T, bool legacy>
	static DrawObjectIndividualNoTransImpl(const T* o, bool noLuaCall);
*/
};

template<typename T>
class ScopedDrawerImpl {
public:
	ScopedDrawerImpl(bool legacy, bool modern) {
		T::Push(legacy, modern);
	}
	~ScopedDrawerImpl() {
		T::Pop();
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::InitStatic()
{
	CModelDrawerConcept::InitStatic();
	//CModelDrawerBase<TDrawerData, TDrawer>::InitInstance is done in TDrawer::InitStatic()

	forceLegacyPath = false;
	drawForward = true;

	modelDrawerData = new TDrawerData{ mtModelDrawer };
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::KillStatic(bool reload)
{
	for (int t = ModelDrawerTypes::MODEL_DRAWER_FFP; t < ModelDrawerTypes::MODEL_DRAWER_CNT; ++t) {
		CModelDrawerBase<TDrawerData, TDrawer>::KillInstance(t);
	}

	spring::SafeDelete(modelDrawerData);
	modelDrawer = nullptr;
	modelDrawerState = nullptr;
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::ForceLegacyPath()
{
	reselectionRequested = true;
	forceLegacyPath = true;
	LOG_L(L_WARNING, "[%s::%s] Using legacy (slow) %s renderer! This is caused by insufficient GPU/driver capabilities or by using of old Lua rendering API", className.data(), __func__, className.data());
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::SelectImplementation(bool forceReselection, bool legacy, bool modern)
{
	if (!reselectionRequested && !forceReselection)
		return;

	reselectionRequested = false;

	if (!advShading) {
		SelectImplementation(ModelDrawerTypes::MODEL_DRAWER_FFP);
		return;
	}

	const auto qualifyDrawerFunc = [legacy, modern](const TDrawer* d, const IModelDrawerState* s) -> bool {
		if (d == nullptr || s == nullptr)
			return false;

		if (s->IsLegacy() && !forceLegacyPath)
			return false;

		if (s->IsLegacy() && !legacy)
			return false;

		if (!s->IsLegacy() && !modern)
			return false;

		if (!s->CanEnable())
			return false;

		return true;
	};

	if (preferedDrawerType >= 0 && preferedDrawerType < ModelDrawerTypes::MODEL_DRAWER_CNT) {
		auto d = modelDrawers[preferedDrawerType];
		auto s = IModelDrawerState::modelDrawerStates[preferedDrawerType];
		if (qualifyDrawerFunc(d, s)) {
			LOG_L(L_INFO, "[%s::%s] Force-switching to %s %s", className.data(), __func__, ModelDrawerNames[preferedDrawerType].data(), className.data());
			SelectImplementation(preferedDrawerType);
			return;
		}
		else {
			LOG_L(L_ERROR, "[%s::%s] Couldn't force-switch to %s %s", className.data(), __func__, ModelDrawerNames[preferedDrawerType].data(), className.data());
			preferedDrawerType = ModelDrawerTypes::MODEL_DRAWER_CNT; //reset;
		}
	}

	int best = ModelDrawerTypes::MODEL_DRAWER_FFP;
	for (int t = ModelDrawerTypes::MODEL_DRAWER_ARB; t < ModelDrawerTypes::MODEL_DRAWER_CNT; ++t) {
		auto d = modelDrawers[t];
		auto s = IModelDrawerState::modelDrawerStates[t];
		if (qualifyDrawerFunc(d, s)) {
			best = t;
		}
	}

	SelectImplementation(best);
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::SelectImplementation(int targetImplementation)
{
	modelDrawer = modelDrawers[targetImplementation];
	assert(modelDrawer);

	modelDrawerState = IModelDrawerState::modelDrawerStates[targetImplementation];
	assert(modelDrawerState);
	assert(modelDrawerState->CanEnable());

	LOG_L(L_INFO, "[%s::%s] Switching to %s %s %s", className.data(), __func__, mtModelDrawer ? "MT" : "ST", ModelDrawerNames[targetImplementation].data(), className.data());
}

/////////////////////////////////////////////////////////////////////////////////////////



template<typename TDrawerData, typename TDrawer, bool legacy>
template<LuaObjType LOT>
inline void CModelDrawerCommon<TDrawerData, TDrawer, legacy>::DrawImpl(bool drawReflection, bool drawRefraction)
{
	if constexpr (legacy) {
		glEnable(GL_ALPHA_TEST);
		sky->SetupFog();
	}

	assert((CCameraHandler::GetActiveCamera())->GetCamType() != CCamera::CAMTYPE_SHADOW);

	// first do the deferred pass; conditional because
	// most of the water renderers use their own FBO's
	if (drawDeferred && !drawReflection && !drawRefraction)
		LuaObjectDrawer::DrawDeferredPass(LUAOBJ_UNIT);

	// now do the regular forward pass
	if (drawForward)
		DrawOpaquePass(false, drawReflection, drawRefraction);

	farTextureHandler->Draw();

	if constexpr (legacy) {
		glDisable(GL_FOG);
		glDisable(GL_TEXTURE_2D);
	}
}

template<typename TDrawerData, typename TDrawer, bool legacy>
template<LuaObjType LOT>
inline void CModelDrawerCommon<TDrawerData, TDrawer, legacy>::DrawShadowPassImpl()
{
	if constexpr (legacy) {
		glColor3f(1.0f, 1.0f, 1.0f);
		glPolygonOffset(1.0f, 1.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);

		glAlphaFunc(GL_GREATER, 0.5f);
		glEnable(GL_ALPHA_TEST);
	}

	CShadowHandler::ShadowGenProgram shadowGenProgram;
	if constexpr (legacy)
		shadowGenProgram = CShadowHandler::SHADOWGEN_PROGRAM_MODEL;
	else
		shadowGenProgram = CShadowHandler::SHADOWGEN_PROGRAM_MODEL_GL4;

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(shadowGenProgram);
	assert(po);
	assert(po->IsValid());
	po->Enable();

	{
		assert((CCameraHandler::GetActiveCamera())->GetCamType() == CCamera::CAMTYPE_SHADOW);
		const auto& quads = modelDrawerData->GetCamVisibleQuads(CCamera::CAMTYPE_SHADOW);

		// 3DO's have clockwise-wound faces and
		// (usually) holes, so disable backface
		// culling for them
		// glDisable(GL_CULL_FACE); Draw(); glEnable(GL_CULL_FACE);

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
			const auto& rdrContProxies = modelDrawerData->GetRdrContProxies(modelType);
			for (int quad : quads) {
				const auto& rdrCntProxy = rdrContProxies[quad];

				// non visible quad
				if (!rdrCntProxy.IsQuadVisible())
					continue;

				// quad has no objects
				if (!rdrCntProxy.HasObjects())
					continue;

				if (modelType == MODELTYPE_3DO)
					glDisable(GL_CULL_FACE);

				// note: just use DrawOpaqueUnits()? would
				// save texture switches needed anyway for
				// UNIT_SHADOW_ALPHA_MASKING
				DrawOpaqueObjectsShadow(rdrCntProxy, modelType);

				if (modelType == MODELTYPE_3DO)
					glEnable(GL_CULL_FACE);

			}
		}
	}

	po->Disable();

	if constexpr (legacy) {
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LOT);
	LuaObjectDrawer::DrawShadowMaterialObjects(LOT, false);
}
