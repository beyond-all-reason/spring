#pragma once

#include <string>
#include <string_view>

#include "ModelRenderData.h"
#include "System/Log/ILog.h"
#include "System/TypeToStr.h"
#include "Rendering/GL/LightHandler.h"

namespace GL { struct GeometryBuffer; }

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
	static bool& WireFrameModeRef() { return wireFrameMode; }
public:
	// lightHandler
	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	// geomBuffer
	const GL::GeometryBuffer* GetGeometryBuffer() const { return geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return geomBuffer; }
protected:
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

		if (modelDrawers[t] == nullptr)
			modelDrawers[t] = new TDrawerDerivative{};
	}
	static void KillInstance(int t) {
		spring::SafeDelete(modelDrawers[t]);
	}

	static void InitStatic();
	static void KillStatic(bool reload);
	static void UpdateStatic() {
		SelectImplementation();
		selectedModelDrawer->Update();
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

	static void SelectImplementation(bool forceReselection = false);
	static void SelectImplementation(int targetImplementation);
public:
	// Draw*

	// Draw() is different in different context. CRTP it
	template <typename ...Args>
	void Draw(Args&&... args) const { static_cast<const TDrawer*>(this)->Draw(std::forward<Args>(args)...); }

	virtual void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawShadowPass() const = 0;
	virtual void DrawAlphaPass() const = 0;
public:
	virtual bool CanEnable() const = 0;

	// Setup Fixed State
	virtual void SetupOpaqueDrawing(bool deferredPass) const = 0;
	virtual void ResetOpaqueDrawing(bool deferredPass) const = 0;

	virtual void SetupAlphaDrawing(bool deferredPass) const = 0;
	virtual void ResetAlphaDrawing(bool deferredPass) const = 0;
public:
	inline static TDrawer* selectedModelDrawer = nullptr;
protected:
	inline static int preferedDrawerType = ModelDrawerTypes::MODEL_DRAWER_CNT; //no preference
	inline static bool mtModelDrawer = true;

	inline static bool reselectionRequested = true;

	inline static bool forceLegacyPath = false;

	inline static bool drawForward = true;
	inline static bool drawDeferred = true;

	inline static TDrawerData* modelDrawerData;
	inline static std::array<TDrawer*, ModelDrawerTypes::MODEL_DRAWER_CNT> modelDrawers = {};
protected:
	static constexpr std::string_view className = spring::type_name<TDrawer>();
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

	modelDrawerData = new TDrawerData{};
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::KillStatic(bool reload)
{
	for (int t = ModelDrawerTypes::MODEL_DRAWER_FFP; t < ModelDrawerTypes::MODEL_DRAWER_CNT; ++t) {
		CModelDrawerBase<TDrawerData, TDrawer>::KillInstance(t);
	}

	spring::SafeDelete(modelDrawerData);
	selectedModelDrawer = nullptr;
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::ForceLegacyPath()
{
	reselectionRequested = true;
	forceLegacyPath = true;
	LOG_L(L_WARNING, "[%s::%s] Using legacy (slow) unit renderer! This is caused by insufficient GPU/driver capabilities or by use of old Lua rendering API", className.data(), __func__);
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::SelectImplementation(bool forceReselection)
{
	if (!reselectionRequested && !forceReselection)
		return;

	reselectionRequested = false;

	if (!advShading) {
		SelectImplementation(ModelDrawerTypes::MODEL_DRAWER_FFP);
		return;
	}

	const auto qualifyDrawerFunc = [](const TDrawer* d) -> bool {
		if (d == nullptr)
			return false;

		if (forceLegacyPath && !d)
			return false;

		if (!d->CanEnable()) {
			return false;
		}

		return true;
	};

	if (preferedDrawerType >= 0 && preferedDrawerType < ModelDrawerTypes::MODEL_DRAWER_CNT) {
		auto* d = modelDrawers[preferedDrawerType];
		if (qualifyDrawerFunc(d)) {
			LOG_L(L_INFO, "[%s::%s] Force-switching to %s ModelDrawer", className.data(), __func__, ModelDrawerNames[preferedDrawerType].data());
			SelectImplementation(preferedDrawerType);
			return;
		}
		else {
			LOG_L(L_ERROR, "[%s::%s] Couldn't force-switch to %s ModelDrawer", className.data(), __func__, ModelDrawerNames[preferedDrawerType].data());
			preferedDrawerType = ModelDrawerTypes::MODEL_DRAWER_CNT; //reset;
		}
	}

	int best = ModelDrawerTypes::MODEL_DRAWER_FFP;
	for (int t = ModelDrawerTypes::MODEL_DRAWER_ARB; t < ModelDrawerTypes::MODEL_DRAWER_CNT; ++t) {
		if (qualifyDrawerFunc(modelDrawers[t])) {
			best = t;
		}
	}

	SelectImplementation(best);
}

template<typename TDrawerData, typename TDrawer>
inline void CModelDrawerBase<TDrawerData, TDrawer>::SelectImplementation(int targetImplementation)
{
	selectedModelDrawer = modelDrawers[targetImplementation];
	assert(selectedModelDrawer);
	assert(selectedModelDrawer->CanEnable());

	LOG_L(L_INFO, "[%s::%s] Switching to %s %s ModelDrawer", className.data(), __func__, mtModelDrawer ? "MT" : "ST", ModelDrawerNames[targetImplementation].data());
}