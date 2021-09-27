#pragma once

#include <string_view>
#include "ModelRenderData.h"

enum ModelDrawerTypes {
	MODEL_DRAWER_FFP =  0, // fixed-function path
	MODEL_DRAWER_ARB =  1, // standard-shader path (ARB)
	MODEL_DRAWER_GLSL = 2, // standard-shader path (GLSL)
	MODEL_DRAWER_GL4 =  3, // GL4-shader path (GLSL)
	MODEL_DRAWER_CNT =  4
};

static const std::string_view ModelDrawerNames[ModelDrawerTypes::MODEL_DRAWER_CNT] = {
	"FFP : fixed-function path",
	"ARB : legacy standard shader path",
	"GLSL: legacy standard shader path",
	"GL4 : modern standard shader path",
};

class CModelRenderConcept {
public:
	CModelRenderConcept() {}
	virtual ~CModelRenderConcept() {}
private:
	inline static bool advShading = false;
	inline static bool wireFrameMode = false;

	inline static bool deferredAllowed = false;

	inline static int preferedDrawerType = ModelDrawerTypes::MODEL_DRAWER_CNT; //no preference
	inline static bool mtModelDrawer = true;
};

template <typename T>
class CModelRenderBase : CModelRenderConcept {
public:
	template<typename T>
	static void InitInstance(int t) {
		if (modelDrawers[t] == nullptr)
			modelDrawers[t] = new T{};
	}
	static void KillInstance(int t) {
		spring::SafeDelete(modelDrawers[t]);
	}
protected:
	inline static bool forceLegacyPath = false;

	inline static bool drawForward = true;
	inline static bool drawDeferred = true;

	inline static CModelRenderDataConcept* modelDrawerData;
private:
	inline static std::array<CModelRenderDataBase<T>*, ModelDrawerTypes::MODEL_DRAWER_CNT> modelDrawers = {};
};

using CUnitRenderBase = CModelRenderBase<CUnit>;
using CFeatureRenderBase = CModelRenderBase<CFeature>;