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
#include "Sim/Misc/TeamHandler.h"
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

bool CFeatureDrawer::ShouldDrawOpaqueFeature(const CFeature* f, bool drawReflection, bool drawRefraction) const
{
	if (modelDrawerData->IsAlpha(f))
		return false;

	if (f->noDraw)
		return false;

	if (f->IsInVoid())
		return false;

	if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(gu->myAllyTeam))
		return false;

	// either PLAYER or UWREFL
	const CCamera* cam = CCameraHandler::GetActiveCamera();

	if (drawRefraction && !f->IsInWater())
		return false;

	if (drawReflection && !CModelDrawerHelper::ObjectVisibleReflection(f->drawMidPos, cam->GetPos(), f->GetDrawRadius()))
		return false;

	if (!cam->InView(f->drawMidPos, f->GetDrawRadius()))
		return false;

	if (f->drawAsFarTex) {
		farTextureHandler->Queue(f);
		return false;
	}

	if (LuaObjectDrawer::AddOpaqueMaterialObject(const_cast<CFeature*>(f), LUAOBJ_FEATURE))
		return false;

	return true;
}

void CFeatureDrawerBase::DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const
{
	const auto* currCamera = CCameraHandler::GetActiveCamera();
	const auto& quads = modelDrawerData->GetCamVisibleQuads(currCamera->GetCamType());

	SetupOpaqueDrawing(deferredPass);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
		const auto& rdrContProxies = modelDrawerData->GetRdrContProxies(modelType);

		CModelDrawerHelper::PushModelRenderState(modelType);

		for (int quad : quads) {
			const auto& rdrCntProxy = rdrContProxies[quad];

			// non visible quad
			if (!rdrCntProxy.IsQuadVisible())
				continue;

			// quad has no objects
			if (!rdrCntProxy.HasObjects())
				continue;

			DrawOpaqueFeatures(rdrCntProxy, modelType, drawReflection, drawRefraction);
		}

		CModelDrawerHelper::PopModelRenderState(modelType);
	}

	ResetOpaqueDrawing(deferredPass);

	// draw all custom'ed features that were bypassed in the loop above
	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawOpaqueMaterialObjects(LUAOBJ_FEATURE, deferredPass);
}

template<bool legacy>
void CFeatureDrawerBase::DrawShadowPassImpl() const
{
	SCOPED_TIMER("CFeatureDrawerBase::DrawShadowPass");
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
				DrawOpaqueFeaturesShadow(rdrCntProxy, modelType);

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

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawShadowMaterialObjects(LUAOBJ_FEATURE, false);
}

template<bool legacy>
void CFeatureDrawerBase::DrawImpl(bool drawReflection, bool drawRefraction) const
{
	SCOPED_TIMER("CFeatureDrawerBase::Draw");
	if constexpr (legacy) {
		sky->SetupFog();
	}

	assert((CCameraHandler::GetActiveCamera())->GetCamType() != CCamera::CAMTYPE_SHADOW);

	// first do the deferred pass; conditional because
	// most of the water renderers use their own FBO's
	if (drawDeferred && !drawReflection && !drawRefraction)
		LuaObjectDrawer::DrawDeferredPass(LUAOBJ_FEATURE);

	// now do the regular forward pass
	if (drawForward)
		DrawOpaquePass(false, drawReflection, drawRefraction);

	farTextureHandler->Draw();

	if constexpr (legacy) {
		glDisable(GL_FOG);
		glDisable(GL_TEXTURE_2D);
	}
}

void CFeatureDrawerBase::Update() const
{
	SCOPED_TIMER("CFeatureDrawerBase::Update");
	modelDrawerData->Update();
}

void CFeatureDrawerLegacy::DrawFeatureNoTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const
{
	if (preList != 0) {
		glCallList(preList);
	}

	DrawFeatureModel(feature, noLuaCall);

	if (postList != 0) {
		glCallList(postList);
	}
}

void CFeatureDrawerLegacy::DrawFeatureTrans(const CFeature* feature, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall) const
{
	glPushMatrix();
	glMultMatrixf(feature->GetTransformMatrixRef());

	DrawFeatureNoTrans(feature, preList, postList, lodCall, noLuaCall);

	glPopMatrix();
}

void CFeatureDrawerLegacy::DrawOpaqueFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const
{
	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		CModelDrawerHelper::BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		for (CFeature* feature : rdrCntProxy.GetObjectBin(i)) {
			DrawOpaqueFeature(feature, drawReflection, drawRefraction);
		}
	}
}

void CFeatureDrawerLegacy::DrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction) const
{
	if (!ShouldDrawOpaqueFeature(f, drawReflection, drawRefraction))
		return;

	// draw the unit with the default (non-Lua) material
	SetTeamColor(f->team);
	DrawFeatureTrans(f, 0, 0, false, false);
}

void CFeatureDrawerLegacy::DrawFeatureModel(const CFeature* feature, bool noLuaCall) const
{
	if (!noLuaCall && feature->luaDraw && eventHandler.DrawFeature(feature))
		return;

	feature->localModel.Draw();
}