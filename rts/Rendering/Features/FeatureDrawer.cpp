/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FeatureDrawer.h"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/ITreeDrawer.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Common/ModelDrawerHelpers.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/Config/ConfigHandler.h"
#include "System/ContainerUtil.h"
#include "System/EventHandler.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

//CONFIG(bool, ShowRezBars).defaultValue(true).headlessValue(false);

static const void SetFeatureAlphaMatSSP(const CFeature* f) { glAlphaFunc(GL_GREATER, f->drawAlpha * 0.5f); }
static const void SetFeatureAlphaMatFFP(const CFeature* f)
{
	const float cols[] = {1.0f, 1.0f, 1.0f, f->drawAlpha};

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cols);
	glColor4fv(cols);

	// hack, sorting objects by distance would look better
	glAlphaFunc(GL_GREATER, f->drawAlpha * 0.5f);
}


typedef const void (*SetFeatureAlphaMatFunc)(const CFeature*);

static const SetFeatureAlphaMatFunc setFeatureAlphaMatFuncs[] = {
	SetFeatureAlphaMatSSP,
	SetFeatureAlphaMatFFP,
};



void CFeatureDrawer::InitStatic()
{
	CModelDrawerBase<CFeatureDrawerData, CFeatureDrawer>::InitStatic();

	LuaObjectDrawer::ReadLODScales(LUAOBJ_FEATURE);

	CFeatureDrawer::InitInstance<CFeatureDrawerFFP >(MODEL_DRAWER_FFP);
	CFeatureDrawer::InitInstance<CFeatureDrawerARB >(MODEL_DRAWER_ARB);
	CFeatureDrawer::InitInstance<CFeatureDrawerGLSL>(MODEL_DRAWER_GLSL);
	CFeatureDrawer::InitInstance<CFeatureDrawerGL4 >(MODEL_DRAWER_GL4);

	SelectImplementation();
}

void CFeatureDrawerCommon::DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const
{
	selectedModelDrawer->SetupOpaqueDrawing(deferredPass);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; modelType++) {
		PushModelRenderState(modelType);
		selectedModelDrawer->DrawOpaqueFeatures(modelType);
		PopModelRenderState(modelType);
	}

	selectedModelDrawer->ResetOpaqueDrawing(deferredPass);

	// draw all custom'ed features that were bypassed in the loop above
	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawOpaqueMaterialObjects(LUAOBJ_FEATURE, deferredPass);
}

void CFeatureDrawerCommon::Update() const
{
	SCOPED_TIMER("CFeatureDrawerCommon::Update");

	const std::function<bool(const CCamera*, const CFeature*)> shouldUpdateFunc = [](const CCamera* cam, const CFeature* feature) -> bool {
		assert(feature->def->drawType == DRAWTYPE_MODEL);

		if (feature->drawFlag == CFeature::FD_NODRAW_FLAG)
			return false;

		if (feature->noDraw)
			return false;

		if (feature->drawAlpha == 0.0f) //?
			return false;

		if (feature->IsInVoid())
			return false;

		if (!feature->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView)
			return false;

		if (cam->GetCamType() == CCamera::CAMTYPE_UWREFL && !CModelDrawerHelper::ObjectVisibleReflection(feature->drawMidPos, cam->GetPos(), feature->GetDrawRadius()))
			return false;

		return cam->InView(feature->drawMidPos, feature->GetDrawRadius());
	};

	UpdateImpl(shouldUpdateFunc);
}