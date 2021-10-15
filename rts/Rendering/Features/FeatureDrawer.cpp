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
#include "Rendering/Models/3DModelVAO.h"
#include "Rendering/Models/MatricesMemStorage.h"
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

bool CFeatureDrawer::ShouldDrawOpaqueFeature(CFeature* f, bool drawReflection, bool drawRefraction)
{
	if (modelDrawerData->IsAlpha(f))
		return false;

	if (f->noDraw)
		return false;

	if (f->IsInVoid())
		return false;

	if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(gu->myAllyTeam))
		return false;

	if (drawRefraction && !f->IsInWater())
		return false;

	// either PLAYER or UWREFL
	const CCamera* cam = CCameraHandler::GetActiveCamera();
	if (drawReflection && !CModelDrawerHelper::ObjectVisibleReflection(f->drawMidPos, cam->GetPos(), f->GetDrawRadius()))
		return false;

	if (!cam->InView(f->drawMidPos, f->GetDrawRadius()))
		return false;

	if (f->drawAsFarTex) {
		farTextureHandler->Queue(f);
		return false;
	}

	if (LuaObjectDrawer::AddOpaqueMaterialObject(f, LUAOBJ_FEATURE))
		return false;

	return true;
}

bool CFeatureDrawer::ShouldDrawAlphaFeature(CFeature* f)
{
	if (!modelDrawerData->IsAlpha(f))
		return false;

	if (f->noDraw)
		return false;

	if (f->IsInVoid())
		return false;

	if (!gu->spectatingFullView && !f->IsInLosForAllyTeam(gu->myAllyTeam))
		return false;

	const CCamera* cam = CCameraHandler::GetActiveCamera();
	if (!cam->InView(f->drawMidPos, f->GetDrawRadius()))
		return false;

	if (f->drawAsFarTex) {
		farTextureHandler->Queue(f);
		return false;
	}

	if (LuaObjectDrawer::AddAlphaMaterialObject(f, LUAOBJ_FEATURE))
		return false;

	return true;
}

bool CFeatureDrawer::ShouldDrawFeatureShadow(CFeature* f)
{
	if (modelDrawerData->IsAlpha(f))
		return false;

	if (f->noDraw)
		return false;

	if (f->IsInVoid())
		return false;

	if (!f->IsInLosForAllyTeam(gu->myAllyTeam) && !gu->spectatingFullView)
		return false;

	// same cutoff as AT; set during SP too
	//if (f->drawAlpha <= 0.1f)
		//return false;

	// either PLAYER or SHADOW or UWREFL
	const CCamera* cam = CCameraHandler::GetActiveCamera();
	if (!cam->InView(f->drawMidPos, f->GetDrawRadius()))
		return false;

	if (f->drawAsFarTex) {
		//farTextureHandler->Queue(f);
		return false;
	}

	if (LuaObjectDrawer::AddShadowMaterialObject(f, LUAOBJ_FEATURE))
		return false;

	return true;
}

void CFeatureDrawer::PushIndividualState(const CFeature* feature, bool deferredPass) const
{
	SetupOpaqueDrawing(false);
	CModelDrawerHelper::PushModelRenderState(feature);
	SetTeamColor(feature->team);
}

void CFeatureDrawer::PopIndividualState(const CFeature* feature, bool deferredPass) const
{
	CModelDrawerHelper::PopModelRenderState(feature);
	ResetOpaqueDrawing(false);
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

void CFeatureDrawerBase::DrawAlphaPass() const
{
	SCOPED_TIMER("CUnitDrawerBase::DrawAlphaPass");

	const auto* currCamera = CCameraHandler::GetActiveCamera();
	const auto& quads = modelDrawerData->GetCamVisibleQuads(currCamera->GetCamType());

	SetupAlphaDrawing(false);

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

			DrawAlphaFeatures(rdrCntProxy, modelType);
		}

		CModelDrawerHelper::PopModelRenderState(modelType);
	}

	ResetAlphaDrawing(false);

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_FEATURE);
	LuaObjectDrawer::DrawAlphaMaterialObjects(LUAOBJ_FEATURE, false);
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
				DrawFeaturesShadow(rdrCntProxy, modelType);

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

void CFeatureDrawerLegacy::DrawIndividual(const CFeature* feature, bool noLuaCall) const
{
	if (LuaObjectDrawer::DrawSingleObject(feature, LUAOBJ_FEATURE /*, noLuaCall*/))
		return;

	// set the full default state
	PushIndividualState(feature, false);
	DrawFeatureTrans(feature, 0, 0, false, noLuaCall);
	PopIndividualState(feature, false);
}

void CFeatureDrawerLegacy::DrawIndividualNoTrans(const CFeature* feature, bool noLuaCall) const
{
	if (LuaObjectDrawer::DrawSingleObjectNoTrans(feature, LUAOBJ_FEATURE /*, noLuaCall*/))
		return;

	PushIndividualState(feature, false);
	DrawFeatureNoTrans(feature, 0, 0, false, noLuaCall);
	PopIndividualState(feature, false);
}

void CFeatureDrawerLegacy::DrawFeaturesShadow(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const
{
	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		// only need to bind the atlas once for 3DO's, but KISS
		assert((modelType != MODELTYPE_3DO) || (rdrCntProxy.GetObjectBinKey(i) == 0));

		//shadowTexBindFuncs[modelType](textureHandlerS3O.GetTexture(mdlRenderer.GetObjectBinKey(i)));
		const auto* texMat = textureHandlerS3O.GetTexture(rdrCntProxy.GetObjectBinKey(i));
		CModelDrawerHelper::modelDrawerHelpers[modelType]->BindShadowTex(texMat);

		for (auto* o : rdrCntProxy.GetObjectBin(i)) {
			DrawOpaqueFeatureShadow(o);
		}

		CModelDrawerHelper::modelDrawerHelpers[modelType]->UnbindShadowTex(nullptr);
	}
}

void CFeatureDrawerLegacy::DrawOpaqueFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const
{
	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		CModelDrawerHelper::BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		for (auto* o : rdrCntProxy.GetObjectBin(i)) {
			DrawOpaqueFeature(o, drawReflection, drawRefraction);
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

void CFeatureDrawerLegacy::DrawAlphaFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const
{
	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		CModelDrawerHelper::BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		for (auto* o : rdrCntProxy.GetObjectBin(i)) {
			DrawAlphaFeature(o);
		}
	}
}

void CFeatureDrawerLegacy::DrawAlphaFeature(CFeature* f) const
{
	if (!ShouldDrawAlphaFeature(f))
		return;

	//SetTeamColor(f->team, float2(alphaValues.x, 1.0f));
	//DrawUnitTrans(unit, 0, 0, false, false);
}

void CFeatureDrawerLegacy::DrawOpaqueFeatureShadow(CFeature* f) const
{
	if (ShouldDrawFeatureShadow(f))
		DrawFeatureTrans(f, 0, 0, false, false);
}

void CFeatureDrawerLegacy::DrawFeatureModel(const CFeature* feature, bool noLuaCall) const
{
	if (!noLuaCall && feature->luaDraw && eventHandler.DrawFeature(feature))
		return;

	feature->localModel.Draw();
}

void CFeatureDrawerGL4::DrawFeaturesShadow(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const
{
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		CModelDrawerHelper::BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		const auto& binObjects = rdrCntProxy.GetObjectBin(i);

		if (!mtModelDrawer) {
			static std::vector<const ObjType*> renderList;
			renderList.resize(binObjects.size());

			for_mt(0, binObjects.size(), [&binObjects, this](const int i) {
				renderList[i] = ShouldDrawFeatureShadow(binObjects[i]) ? binObjects[i] : nullptr;
			});

			for (auto* o : renderList) {
				if (!o)
					continue;

				smv.AddToSubmission(o);
			}
		}
		else {
			for (auto* o : binObjects) {
				if (!ShouldDrawFeatureShadow(o))
					continue;

				smv.AddToSubmission(o);
			}
		}
		smv.Submit(GL_TRIANGLES, false);

		CModelDrawerHelper::modelDrawerHelpers[modelType]->UnbindShadowTex(nullptr);
	}

	smv.Unbind();
}

void CFeatureDrawerGL4::DrawOpaqueFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const
{
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	modelDrawerState->SetColorMultiplier();

	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		CModelDrawerHelper::BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		if (!mtModelDrawer) {
			for (auto* o : rdrCntProxy.GetObjectBin(i)) {
				//DrawOpaqueUnit(unit, drawReflection, drawRefraction);
				if (!ShouldDrawOpaqueFeature(o, drawReflection, drawRefraction))
					continue;

				smv.AddToSubmission(o);
			}
		}
		else {
			const auto& bin = rdrCntProxy.GetObjectBin(i);
			static std::vector<const ObjType*> renderList;
			renderList.resize(bin.size());
			for_mt(0, renderList.size(), [this, &bin, drawReflection, drawRefraction](int k) {
				auto* o = bin[k];
				renderList[k] = ShouldDrawOpaqueFeature(o, drawReflection, drawRefraction) ? o : nullptr;
				});

			for (const auto* o : renderList)
				if (o)
					smv.AddToSubmission(o);

		}
		smv.Submit(GL_TRIANGLES, false);
	}

	smv.Unbind();
}

void CFeatureDrawerGL4::DrawAlphaFeatures(const CFeatureRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const
{
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	modelDrawerState->SetColorMultiplier(IModelDrawerState::alphaValues.x);

	//main cloaked alpha pass
	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		CModelDrawerHelper::BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		const auto& binObjects = rdrCntProxy.GetObjectBin(i);

		if (!mtModelDrawer) {
			for (auto* o : binObjects) {
				if (!ShouldDrawAlphaFeature(o))
					continue;

				smv.AddToSubmission(o);
			}
		}
		else {
			static std::vector<const ObjType*> renderList;
			renderList.resize(binObjects.size());

			for_mt(0, binObjects.size(), [&binObjects, this](const int i) {
				renderList[i] = ShouldDrawAlphaFeature(binObjects[i]) ? binObjects[i] : nullptr;
				});

			for (auto* o : renderList) {
				if (!o)
					continue;

				smv.AddToSubmission(o);
			}
		}

		smv.Submit(GL_TRIANGLES, false);
	}
	smv.Unbind();
}
