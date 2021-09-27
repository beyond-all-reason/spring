/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <vector>

#include "UnitDrawer.h"
#include "UnitDrawerState.hpp"

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/UI/MiniMap.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include "Rendering/Env/ISky.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/Env/CubeMapHandler.h"
#include "Rendering/FarTextureHandler.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Colors.h"
#include "Rendering/IconHandler.h"
#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Rendering/Models/3DModelVAO.h"
#include "Rendering/Models/MatricesMemStorage.h"

#include "Sim/Features/Feature.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
//#include "System/FileSystem/FileHandler.h"

#include "System/StringUtil.h"
#include "System/MemPoolTypes.h"
#include "System/SpringMath.h"

#include "System/Threading/ThreadPool.h"

#define UNIT_SHADOW_ALPHA_MASKING

CONFIG(int, UnitLodDist).defaultValue(1000).headlessValue(0);
CONFIG(int, UnitIconDist).defaultValue(200).headlessValue(0);
CONFIG(float, UnitIconScaleUI).defaultValue(1.0f).minimumValue(0.5f).maximumValue(2.0f);
CONFIG(float, UnitIconFadeStart).defaultValue(3000.0f).minimumValue(1.0f).maximumValue(10000.0f);
CONFIG(float, UnitIconFadeVanish).defaultValue(1000.0f).minimumValue(1.0f).maximumValue(10000.0f);
CONFIG(float, UnitTransparency).defaultValue(0.7f);
CONFIG(bool, UnitIconsAsUI).defaultValue(false).description("Draw unit icons like it is an UI element and not like unit's LOD.");
CONFIG(bool, UnitIconsHideWithUI).defaultValue(false).description("Hide unit icons when UI is hidden.");

CONFIG(int, MaxDynamicModelLights)
	.defaultValue(1)
	.minimumValue(0);

CONFIG(bool, AdvUnitShading).defaultValue(true).headlessValue(false).safemodeValue(false).description("Determines whether specular highlights and other lighting effects are rendered for units.");




CUnitDrawer* unitDrawer = nullptr;

/***********************************************************************/

class CUnitDrawerHelper
{
public:
	virtual void BindOpaqueTex(const CS3OTextureHandler::S3OTexMat * textureMat) const = 0;
	virtual void UnbindOpaqueTex(const CS3OTextureHandler::S3OTexMat * textureMat) const = 0;
	virtual void BindShadowTex(const CS3OTextureHandler::S3OTexMat * textureMat) const = 0;
	virtual void UnbindShadowTex(const CS3OTextureHandler::S3OTexMat * textureMat)  const = 0;
	virtual void PushRenderState() const = 0;
	virtual void PopRenderState() const = 0;
public:
	static void EnableTexturesCommon() {
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);

		if (shadowHandler.ShadowsLoaded())
			shadowHandler.SetupShadowTexSampler(GL_TEXTURE2, true);

		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetEnvReflectionTextureID());

		glActiveTexture(GL_TEXTURE4);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapHandler.GetSpecularTextureID());

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
	}

	static void DisableTexturesCommon() {
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);

		if (shadowHandler.ShadowsLoaded())
			shadowHandler.ResetShadowTexSampler(GL_TEXTURE2, true);

		glActiveTexture(GL_TEXTURE3);
		glDisable(GL_TEXTURE_CUBE_MAP);

		glActiveTexture(GL_TEXTURE4);
		glDisable(GL_TEXTURE_CUBE_MAP);

		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
	}

	static void PushTransform(const CCamera* cam) {
		// set model-drawing transform; view is combined with projection
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glMultMatrixf(cam->GetViewMatrix());
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
	}

	static void PopTransform() {
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	static float4 GetTeamColor(int team, float alpha) {
		assert(teamHandler.IsValidTeam(team));

		const   CTeam* t = teamHandler.Team(team);
		const uint8_t* c = t->color;

		return (float4(c[0] / 255.0f, c[1] / 255.0f, c[2] / 255.0f, alpha));
	}

	static void LoadUnitExplosionGenerators() {
		using F = decltype(&UnitDef::AddModelExpGenID);
		using T = decltype(UnitDef::modelCEGTags);

		const auto LoadGenerators = [](UnitDef* ud, const F addExplGenID, const T& explGenTags, const char* explGenPrefix) {
			for (const auto& explGenTag : explGenTags) {
				if (explGenTag[0] == 0)
					break;

				// build a contiguous range of valid ID's
				(ud->*addExplGenID)(explGenHandler.LoadGeneratorID(explGenTag, explGenPrefix));
			}
		};

		for (uint32_t i = 0, n = unitDefHandler->NumUnitDefs(); i < n; i++) {
			UnitDef* ud = const_cast<UnitDef*>(unitDefHandler->GetUnitDefByID(i + 1));

			// piece- and crash-generators can only be custom so the prefix is not required to be given game-side
			LoadGenerators(ud, &UnitDef::AddModelExpGenID, ud->modelCEGTags, "");
			LoadGenerators(ud, &UnitDef::AddPieceExpGenID, ud->pieceCEGTags, CEG_PREFIX_STRING);
			LoadGenerators(ud, &UnitDef::AddCrashExpGenID, ud->crashCEGTags, CEG_PREFIX_STRING);
		}
	}

	static void DIDResetPrevProjection(bool toScreen)
	{
		if (!toScreen)
			return;

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glPushMatrix();
	}

	static void DIDResetPrevModelView()
	{
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glPushMatrix();
	}

	static bool DIDCheckMatrixMode(int wantedMode)
	{
		#if 1
			int matrixMode = 0;
			glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
			return (matrixMode == wantedMode);
		#else
			return true;
		#endif
	}

	static inline float GetUnitIconScale(const CUnit* unit) {
		float scale = unit->myIcon->GetSize();

		if (!minimap->UseUnitIcons())
			return scale;
		if (!unit->myIcon->GetRadiusAdjust())
			return scale;

		const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
		const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
		const bool unitVisible = ((losStatus & LOS_INLOS) || ((losStatus & LOS_INRADAR) && ((losStatus & prevMask) == prevMask)));

		if ((unitVisible || gu->spectatingFullView)) {
			scale *= (unit->radius / unit->myIcon->GetRadiusScale());
		}

		return scale;
	}
public:
	template<typename T>
	static const CUnitDrawerHelper* GetInstance() {
		static const T instance;
		return &instance;
	}
public:
	static const std::array<const CUnitDrawerHelper*, MODELTYPE_CNT> unitDrawerHelpers;
};

class CUnitDrawerHelper3DO : public CUnitDrawerHelper
{
public:
	virtual void BindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {/*handled in PushRenderState()*/}
	virtual void UnbindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {/*handled in PopRenderState()*/}
	virtual void BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureHandler3DO.GetAtlasTex2ID());
	}
	virtual void UnbindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
	virtual void PushRenderState() const override {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textureHandler3DO.GetAtlasTex2ID());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureHandler3DO.GetAtlasTex1ID());

		glDisable(GL_CULL_FACE);
	}
	virtual void PopRenderState() const override {
		glEnable(GL_CULL_FACE);
	}
};

class CUnitDrawerHelperS3O : public CUnitDrawerHelper
{
public:
	virtual void BindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textureMat->tex2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureMat->tex1);
	}
	virtual void UnbindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	virtual void BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureMat->tex2);
	}
	virtual void UnbindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
	virtual void PushRenderState() const override {/* no need for primitve restart*/};
	virtual void PopRenderState() const override {/* no need for primitve restart*/};
};

class CUnitDrawerHelperASS : public CUnitDrawerHelper
{
public:
	virtual void BindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textureMat->tex2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureMat->tex1);
	}
	virtual void UnbindOpaqueTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	virtual void BindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureMat->tex2);
	}
	virtual void UnbindShadowTex(const CS3OTextureHandler::S3OTexMat* textureMat) const override {
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
	}
	virtual void PushRenderState() const override {};
	virtual void PopRenderState() const override {};
};


const std::array<const CUnitDrawerHelper*, MODELTYPE_CNT> CUnitDrawerHelper::unitDrawerHelpers = {
	CUnitDrawerHelper::GetInstance<CUnitDrawerHelper3DO>(),
	CUnitDrawerHelper::GetInstance<CUnitDrawerHelperS3O>(),
	CUnitDrawerHelper::GetInstance<CUnitDrawerHelperASS>(),
};


/***********************************************************************/


void CUnitDrawer::InitStatic()
{
	LuaObjectDrawer::ReadLODScales(LUAOBJ_UNIT);

	alphaValues.x = std::max(0.11f, std::min(1.0f, 1.0f - configHandler->GetFloat("UnitTransparency")));
	alphaValues.y = std::min(1.0f, alphaValues.x + 0.1f);
	alphaValues.z = std::min(1.0f, alphaValues.x + 0.2f);
	alphaValues.w = std::min(1.0f, alphaValues.x + 0.4f);

	CUnitDrawerHelper::LoadUnitExplosionGenerators();

	forceLegacyPath = false;
	wireFrameMode = false;

	drawForward = true;

	lightHandler.Init(2U, configHandler->GetInt("MaxDynamicModelLights"));

	advShading = configHandler->GetBool("AdvUnitShading") && cubeMapHandler.Init();
	deferredAllowed = configHandler->GetBool("AllowDeferredModelRendering");

	// shared with FeatureDrawer!
	geomBuffer = LuaObjectDrawer::GetGeometryBuffer();
	deferredAllowed &= geomBuffer->Valid();

	unitDrawerData = new CUnitDrawerData{};

	CUnitDrawer::InitInstance<CUnitDrawerFFP >(UNIT_DRAWER_FFP );
	CUnitDrawer::InitInstance<CUnitDrawerARB >(UNIT_DRAWER_ARB );
	CUnitDrawer::InitInstance<CUnitDrawerGLSL>(UNIT_DRAWER_GLSL);
	CUnitDrawer::InitInstance<CUnitDrawerGL4 >(UNIT_DRAWER_GL4 );

	SelectImplementation();
}

void CUnitDrawer::KillStatic(bool reload)
{
	for (int t = UNIT_DRAWER_FFP; t < UNIT_DRAWER_CNT; ++t) {
		CUnitDrawer::KillInstance(t);
	}

	spring::SafeDelete(unitDrawerData);

	unitDrawer = nullptr;

	cubeMapHandler.Free();

	geomBuffer = nullptr;
}

void CUnitDrawer::ForceLegacyPath()
{
	reselectionRequested = true;
	forceLegacyPath = true;
	LOG_L(L_WARNING, "[CUnitDrawer::%s] Using legacy (slow) unit renderer! This is caused by insufficient GPU/driver capabilities or by use of old Lua rendering API", __func__);
}

void CUnitDrawer::SelectImplementation(bool forceReselection)
{
	if (!reselectionRequested && !forceReselection)
		return;

	reselectionRequested = false;

	if (!advShading) {
		SelectImplementation(UnitDrawerTypes::UNIT_DRAWER_FFP);
		return;
	}

	const auto qualifyDrawerFunc = [](const CUnitDrawer* ud) -> bool {
		if (ud == nullptr)
			return false;

		if (forceLegacyPath && !ud)
			return false;

		if (!ud->CanEnable()) {
			return false;
		}

		return true;
	};

	if (preferedDrawerType >= 0 && preferedDrawerType < UnitDrawerTypes::UNIT_DRAWER_CNT) {
		auto* ud = unitDrawers[preferedDrawerType];
		if (qualifyDrawerFunc(ud)) {
			LOG_L(L_INFO, "[CUnitDrawer::%s] Force-switching to %s UnitDrawer", __func__, UnitDrawerNames[preferedDrawerType].c_str());
			SelectImplementation(preferedDrawerType);
			return;
		} else {
			LOG_L(L_ERROR, "[CUnitDrawer::%s] Couldn't force-switch to %s UnitDrawer", __func__, UnitDrawerNames[preferedDrawerType].c_str());
			preferedDrawerType = UnitDrawerTypes::UNIT_DRAWER_CNT; //reset;
		}
	}

	int best = UnitDrawerTypes::UNIT_DRAWER_FFP;
	for (int t = UnitDrawerTypes::UNIT_DRAWER_ARB; t < UnitDrawerTypes::UNIT_DRAWER_CNT; ++t) {
		if (qualifyDrawerFunc(unitDrawers[t])) {
			best = t;
		}
	}

	SelectImplementation(best);
}

void CUnitDrawer::SelectImplementation(int targetImplementation)
{
	//selectedImplementation = targetImplementation;

	unitDrawer = unitDrawers[targetImplementation];
	assert(unitDrawer);
	assert(unitDrawer->CanEnable());

	LOG_L(L_INFO, "[CUnitDrawer::%s] Switching to %s %s UnitDrawer", __func__, mtModelDrawer ? "MT" : "ST", UnitDrawerNames[targetImplementation].c_str());
}

void CUnitDrawer::UpdateStatic()
{
	SelectImplementation();
	unitDrawer->Update();
}


void CUnitDrawer::SunChangedStatic()
{
	for (auto ud : unitDrawers) {
		if (ud == nullptr)
			continue;
		if (!ud->CanEnable())
			continue;

		ud->SunChanged();
	}
}

bool CUnitDrawer::SetTeamColour(int team, const float2 alpha) const
{
	// need this because we can be called by no-team projectiles
	if (!teamHandler.IsValidTeam(team))
		return false;

	// should be an assert, but projectiles (+FlyingPiece) would trigger it
	if (shadowHandler.InShadowPass())
		return false;

	return true;
}

void CUnitDrawer::BindModelTypeTexture(int mdlType, int texType)
{
	const auto texMat = textureHandlerS3O.GetTexture(texType);

	if (shadowHandler.InShadowPass())
		CUnitDrawerHelper::unitDrawerHelpers[mdlType]->BindShadowTex(texMat);
	else
		CUnitDrawerHelper::unitDrawerHelpers[mdlType]->BindOpaqueTex(texMat);
}

void CUnitDrawer::PushModelRenderState(int mdlType)
{
	assert(CUnitDrawerHelper::unitDrawerHelpers[mdlType]);
	CUnitDrawerHelper::unitDrawerHelpers[mdlType]->PushRenderState();
}

void CUnitDrawer::PushModelRenderState(const S3DModel* m)
{
	PushModelRenderState(m->type);
	BindModelTypeTexture(m->type, m->textureType);
}

void CUnitDrawer::PushModelRenderState(const CSolidObject* o) { PushModelRenderState(o->model); }

void CUnitDrawer::PopModelRenderState(int mdlType)
{
	assert(CUnitDrawerHelper::unitDrawerHelpers[mdlType]);
	CUnitDrawerHelper::unitDrawerHelpers[mdlType]->PopRenderState();
}

void CUnitDrawer::PopModelRenderState(const S3DModel*     m) { PopModelRenderState(m->type); }
void CUnitDrawer::PopModelRenderState(const CSolidObject* o) { PopModelRenderState(o->model); }

bool CUnitDrawer::ObjectVisibleReflection(const float3 objPos, const float3 camPos, float maxRadius)
{
	if (objPos.y < 0.0f)
		return (CGround::GetApproximateHeight(objPos.x, objPos.z, false) <= maxRadius);

	const float dif = objPos.y - camPos.y;

	float3 zeroPos;
	zeroPos += (camPos * ( objPos.y / dif));
	zeroPos += (objPos * (-camPos.y / dif));

	return (CGround::GetApproximateHeight(zeroPos.x, zeroPos.z, false) <= maxRadius);
}

bool CUnitDrawer::CanDrawOpaqueUnit(
	const CUnit* unit,
	bool drawReflection,
	bool drawRefraction
) const {
	if (unitDrawerData->IsAlpha(unit))
		return false;

	if (unit == (drawReflection ? nullptr : (gu->GetMyPlayer())->fpsController.GetControllee()))
		return false;

	if (unit->noDraw)
		return false;

	if (unit->IsInVoid())
		return false;

	// unit will be drawn as icon instead
	if (unit->isIcon)
		return false;

	if (!(unit->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView)
		return false;

	// either PLAYER or UWREFL
	const CCamera* cam = CCameraHandler::GetActiveCamera();

	if (drawRefraction && !unit->IsInWater())
		return false;

	if (drawReflection && !ObjectVisibleReflection(unit->drawMidPos, cam->GetPos(), unit->GetDrawRadius()))
		return false;

	return (cam->InView(unit->drawMidPos, unit->GetDrawRadius()));
}

bool CUnitDrawer::ShouldDrawOpaqueUnit(const CUnit* unit, bool drawReflection, bool drawRefraction) const
{
	if (!CanDrawOpaqueUnit(unit, drawReflection, drawRefraction))
		return false;

	if ((unit->pos).SqDistance(camera->GetPos()) > (unit->sqRadius * unitDrawerData->unitDrawDistSqr)) {
		farTextureHandler->Queue(unit);
		return false;
	}

	if (LuaObjectDrawer::AddOpaqueMaterialObject(const_cast<CUnit*>(unit), LUAOBJ_UNIT))
		return false;

	return true;
}

bool CUnitDrawer::ShouldDrawAlphaUnit(CUnit* unit) const
{
	if (!unitDrawerData->IsAlpha(unit))
		return false;

	if (!camera->InView(unit->drawMidPos, unit->GetDrawRadius()))
		return false;

	if (LuaObjectDrawer::AddAlphaMaterialObject(unit, LUAOBJ_UNIT))
		return false;

	if (unit->isIcon)
		return false;

	if (!(unit->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView)
		return false;

	return true;
}

bool CUnitDrawer::CanDrawOpaqueUnitShadow(const CUnit* unit) const
{
	if (unitDrawerData->IsAlpha(unit))
		return false;

	if (unit->noDraw)
		return false;

	if (unit->IsInVoid())
		return false;

	// no shadow if unit is already an icon from player's POV
	if (unit->isIcon)
		return false;

	if (unit->isCloaked)
		return false;

	const CCamera* cam = CCameraHandler::GetActiveCamera();

	const bool unitInLOS = ((unit->losStatus[gu->myAllyTeam] & LOS_INLOS) || gu->spectatingFullView);
	const bool unitInView = cam->InView(unit->drawMidPos, unit->GetDrawRadius());

	return (unitInLOS && unitInView);
}

bool CUnitDrawer::ShouldDrawOpaqueUnitShadow(CUnit* unit) const
{
	if (!CanDrawOpaqueUnitShadow(unit))
		return false;

	if (LuaObjectDrawer::AddShadowMaterialObject(unit, LUAOBJ_UNIT))
		return false;

	return true;
}

/***********************************************************************/

void CUnitDrawerBase::DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const
{
	const auto* currCamera = CCameraHandler::GetActiveCamera();
	const auto& quads = unitDrawerData->GetCamVisibleQuads(currCamera->GetCamType());

	SetupOpaqueDrawing(deferredPass);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
		const auto& rdrContProxies = unitDrawerData->GetRdrContProxies(modelType);

		PushModelRenderState(modelType);

		for (int quad : quads) {
			const auto& rdrCntProxy = rdrContProxies[quad];

			// non visible quad
			if (!rdrCntProxy.IsQuadVisible())
				continue;

			// quad has no objects
			if (!rdrCntProxy.HasObjects())
				continue;

			DrawOpaqueUnits(rdrCntProxy, modelType, drawReflection, drawRefraction);
		}

		DrawOpaqueAIUnits(modelType);
		PopModelRenderState(modelType);
	}

	ResetOpaqueDrawing(deferredPass);

	// draw all custom'ed units that were bypassed in the loop above
	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_UNIT);
	LuaObjectDrawer::DrawOpaqueMaterialObjects(LUAOBJ_UNIT, deferredPass);
}

void CUnitDrawerBase::DrawAlphaPass() const
{
	SCOPED_TIMER("CUnitDrawerBase::DrawAlphaPass");

	const auto* currCamera = CCameraHandler::GetActiveCamera();
	const auto& quads = unitDrawerData->GetCamVisibleQuads(currCamera->GetCamType());

	SetupAlphaDrawing(false);

	for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
		const auto& rdrContProxies = unitDrawerData->GetRdrContProxies(modelType);

		PushModelRenderState(modelType);

		for (int quad : quads) {
			const auto& rdrCntProxy = rdrContProxies[quad];

			// non visible quad
			if (!rdrCntProxy.IsQuadVisible())
				continue;

			// quad has no objects
			if (!rdrCntProxy.HasObjects())
				continue;

			DrawAlphaUnits(rdrCntProxy, modelType);
		}

		DrawAlphaAIUnits(modelType);
		PopModelRenderState(modelType);
	}

	ResetAlphaDrawing(false);

	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_UNIT);
	LuaObjectDrawer::DrawAlphaMaterialObjects(LUAOBJ_UNIT, false);
}

void CUnitDrawerBase::Update() const
{
	SCOPED_TIMER("CUnitDrawerBase::Update");
	unitDrawerData->Update();

	//TODO: move into unitDrawerData->Update() ?

	const static auto shouldUpdateFunc = [](CCamera* cam, CUnit* unit) -> bool {
		if (unit->noDraw)
			return false;

		if (unit->IsInVoid())
			return false;

		// unit will be drawn as icon instead
		if (unit->isIcon)
			return false;

		if (!(unit->losStatus[gu->myAllyTeam] & LOS_INLOS) && !gu->spectatingFullView)
			return false;

		if (cam->GetCamType() == CCamera::CAMTYPE_UWREFL && !ObjectVisibleReflection(unit->drawMidPos, cam->GetPos(), unit->GetDrawRadius()))
			return false;

		return cam->InView(unit->drawMidPos, unit->GetDrawRadius());
	};

	const static auto matUpdateFunc = [this](CUnit* unit) {
		auto& smma = unitDrawerData->GetObjectMatricesMemAlloc(unit);
		smma[0] = unit->GetTransformMatrix();

		for (int i = 0; i < unit->localModel.pieces.size(); ++i) {
			auto& lmp = unit->localModel.pieces[i];
			smma[i + 1] = lmp.scriptSetVisible ? lmp.GetModelSpaceMatrix() : CMatrix44f::Zero();
		}
	};

	//Experimental: do not include CAMTYPE_SHADOW. A little cheating to reduce the number of processed units
	// Replace CAMTYPE_SHADOW -> CAMTYPE_ENVMAP in case missing shadow hits back
	for (uint32_t camType = CCamera::CAMTYPE_PLAYER; camType < CCamera::CAMTYPE_SHADOW; ++camType) {
		CCamera* cam = CCameraHandler::GetCamera(camType);
		const auto& quads = unitDrawerData->GetCamVisibleQuads(camType);

		static std::vector<CUnit*> updateList;
		updateList.clear();

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
			const auto& rdrContProxies = unitDrawerData->GetRdrContProxies(modelType);

			for (int quad : quads) {
				const auto& rdrCntProxy = rdrContProxies[quad];

				// non visible quad
				if (!rdrCntProxy.IsQuadVisible())
					continue;

				// quad has no objects
				if (!rdrCntProxy.HasObjects())
					continue;

				for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
					const auto& bin = rdrCntProxy.GetObjectBin(i);
					for (CUnit* unit : bin)
						updateList.emplace_back(unit);
				}
			}
		}
		spring::VectorSortUnique(updateList);

		if (mtModelDrawer) {
			for_mt(0, updateList.size(), [cam](const int k) {
				CUnit* unit = updateList[k];
				if (shouldUpdateFunc(cam, unit))
					matUpdateFunc(unit);
				});
		}
		else {
			for (CUnit* unit : updateList) {
				if (shouldUpdateFunc(cam, unit))
					matUpdateFunc(unit);
			}
		}
	}

	for (CUnit* unit : GetUnsortedUnits()) {
		if (unit->alwaysUpdateMat)
			matUpdateFunc(unit);
	}
}

template<bool legacy>
void CUnitDrawerBase::DrawShadowPassImpl() const
{
	SCOPED_TIMER("CUnitDrawerBase::DrawShadowPass");
	if constexpr (legacy) {
		glColor3f(1.0f, 1.0f, 1.0f);
		glPolygonOffset(1.0f, 1.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);

		#ifdef UNIT_SHADOW_ALPHA_MASKING
			glAlphaFunc(GL_GREATER, 0.5f);
			glEnable(GL_ALPHA_TEST);
		#endif
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
		const auto& quads = unitDrawerData->GetCamVisibleQuads(CCamera::CAMTYPE_SHADOW);

		// 3DO's have clockwise-wound faces and
		// (usually) holes, so disable backface
		// culling for them
		// glDisable(GL_CULL_FACE); Draw(); glEnable(GL_CULL_FACE);

		for (int modelType = MODELTYPE_3DO; modelType < MODELTYPE_CNT; ++modelType) {
			const auto& rdrContProxies = unitDrawerData->GetRdrContProxies(modelType);
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
				DrawOpaqueUnitsShadow(rdrCntProxy, modelType);

				if (modelType == MODELTYPE_3DO)
					glEnable(GL_CULL_FACE);

			}
		}
	}

	po->Disable();

	if constexpr (legacy) {
		#ifdef UNIT_SHADOW_ALPHA_MASKING
				glDisable(GL_ALPHA_TEST);
		#endif

		glDisable(GL_POLYGON_OFFSET_FILL);
	}


	LuaObjectDrawer::SetDrawPassGlobalLODFactor(LUAOBJ_UNIT);
	LuaObjectDrawer::DrawShadowMaterialObjects(LUAOBJ_UNIT, false);
}

template<bool legacy>
void CUnitDrawerBase::DrawImpl(bool drawReflection, bool drawRefraction) const
{
	SCOPED_TIMER("CUnitDrawerBase::Draw");
	if constexpr(legacy)
		sky->SetupFog();

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
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_TEXTURE_2D);
	}
}

/***********************************************************************/

void CUnitDrawerLegacy::SetupOpaqueDrawing(bool deferredPass) const
{
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireFrameMode + GL_FILL * (1 - wireFrameMode));

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_ALPHA_TEST);

	Enable(deferredPass, false);
}

void CUnitDrawerLegacy::ResetOpaqueDrawing(bool deferredPass) const
{
	Disable(deferredPass);
	glPopAttrib();
}

void CUnitDrawerLegacy::SetupAlphaDrawing(bool deferredPass) const
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireFrameMode + GL_FILL * (1 - wireFrameMode));

	Enable(/*deferredPass always false*/ false, true);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
	glDepthMask(GL_FALSE);
}

void CUnitDrawerLegacy::ResetAlphaDrawing(bool deferredPass) const
{
	Disable(/*deferredPass*/ false);
	glPopAttrib();
}

void CUnitDrawerLegacy::DrawUnitModel(const CUnit* unit, bool noLuaCall) const
{
	if (!noLuaCall && unit->luaDraw && eventHandler.DrawUnit(unit))
		return;

	unit->localModel.Draw();
}

void CUnitDrawerLegacy::DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const
{
	const bool noNanoDraw = lodCall || !unit->beingBuilt || !unit->unitDef->showNanoFrame;
	const bool shadowPass = shadowHandler.InShadowPass();

	if (preList != 0) {
		glCallList(preList);
	}

	// if called from LuaObjectDrawer, unit has a custom material
	//
	// we want Lua-material shaders to have full control over build
	// visualisation, so keep it simple and make LOD-calls draw the
	// full model
	//
	// NOTE: "raw" calls will no longer skip DrawUnitBeingBuilt
	//

	//drawModelFuncs[std::max(noNanoDraw * 2, shadowPass)](unit, noLuaCall);
	if (noNanoDraw)
		DrawUnitModel(unit, noLuaCall);
	else {
		if (shadowPass)
			DrawUnitModelBeingBuiltShadow(unit, noLuaCall);
		else
			DrawUnitModelBeingBuiltOpaque(unit, noLuaCall);
	}


	if (postList != 0) {
		glCallList(postList);
	}
}

void CUnitDrawerLegacy::DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const
{
	glPushMatrix();
	glMultMatrixf(unit->GetTransformMatrix());

	DrawUnitNoTrans(unit, preList, postList, lodCall, noLuaCall);

	glPopMatrix();
}

void CUnitDrawerLegacy::DrawUnitMiniMapIcons() const
{
	CVertexArray* va = GetVertexArray();

	for (const auto& [icon, units] : unitDrawerData->GetUnitsByIcon()) {

		if (icon == nullptr)
			continue;
		if (units.empty())
			continue;

		va->Initialize();
		va->EnlargeArrays(units.size() * 4, 0, VA_SIZE_2DTC);
		icon->BindTexture();

		for (const CUnit* unit : units) {
			assert(unit->myIcon == icon);
			DrawUnitMiniMapIcon(unit, va);
		}

		va->DrawArray2dTC(GL_QUADS);
	}
}

void CUnitDrawerLegacy::DrawUnitIcons() const
{
	// draw unit icons and radar blips
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.05f);

	// A2C effectiveness is limited below four samples
	if (globalRendering->msaaLevel >= 4)
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB);

	for (CUnit* u : unitDrawerData->GetIconUnits()) {
		const unsigned short closBits = (u->losStatus[gu->myAllyTeam] & (LOS_INLOS));
		const unsigned short plosBits = (u->losStatus[gu->myAllyTeam] & (LOS_PREVLOS | LOS_CONTRADAR));

		DrawIcon(u, !gu->spectatingFullView && closBits == 0 && plosBits != (LOS_PREVLOS | LOS_CONTRADAR));
	}

	glPopAttrib();
}

void CUnitDrawerLegacy::DrawUnitIconsScreen() const
{
	if (game->hideInterface && unitDrawerData->iconHideWithUI)
		return;

	// draw unit icons and radar blips
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.05f);

	CVertexArray* va = GetVertexArray();

	for (const auto& [icon, units] : unitDrawerData->GetUnitsByIcon())
	{

		if (icon == nullptr)
			continue;
		if (units.empty())
			continue;

		va->Initialize();
		va->EnlargeArrays(units.size() * 4, 0, VA_SIZE_2DTC);
		icon->BindTexture();

		for (const CUnit* unit : units)
		{
			if (unit->noDraw)
				continue;
			if (unit->IsInVoid())
				continue;
			if (unit->health <= 0 || unit->beingBuilt)
				continue;

			const unsigned short closBits = (unit->losStatus[gu->myAllyTeam] & (LOS_INLOS));
			const unsigned short plosBits = (unit->losStatus[gu->myAllyTeam] & (LOS_PREVLOS | LOS_CONTRADAR));

			assert(unit->myIcon == icon);
			DrawIconScreenArray(unit, icon, !gu->spectatingFullView && closBits == 0 && plosBits != (LOS_PREVLOS | LOS_CONTRADAR), unitDrawerData->iconZoomDist, va);
		}

		va->DrawArray2dTC(GL_QUADS);
	}
	glPopAttrib();
}

void CUnitDrawerLegacy::DrawOpaqueUnitsShadow(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const
{
	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		// only need to bind the atlas once for 3DO's, but KISS
		assert((modelType != MODELTYPE_3DO) || (rdrCntProxy.GetObjectBinKey(i) == 0));

		//shadowTexBindFuncs[modelType](textureHandlerS3O.GetTexture(mdlRenderer.GetObjectBinKey(i)));
		const auto* texMat = textureHandlerS3O.GetTexture(rdrCntProxy.GetObjectBinKey(i));
		CUnitDrawerHelper::unitDrawerHelpers[modelType]->BindShadowTex(texMat);

		for (CUnit* unit : rdrCntProxy.GetObjectBin(i)) {
			DrawOpaqueUnitShadow(unit);
		}

		CUnitDrawerHelper::unitDrawerHelpers[modelType]->UnbindShadowTex(nullptr);
	}
}

void CUnitDrawerLegacy::DrawOpaqueUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const
{
	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		for (CUnit* unit : rdrCntProxy.GetObjectBin(i)) {
			DrawOpaqueUnit(unit, drawReflection, drawRefraction);
		}
	}
}

void CUnitDrawerLegacy::DrawAlphaUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const
{
	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		for (CUnit* unit : rdrCntProxy.GetObjectBin(i)) {
			DrawAlphaUnit(unit, modelType, false);
		}
	}

	// living and dead ghosted buildings
	if (!gu->spectatingFullView)
		DrawGhostedBuildings(modelType);
}

void CUnitDrawerLegacy::DrawOpaqueAIUnits(int modelType) const
{
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpOpaqueUnits = unitDrawerData->GetTempOpaqueDrawUnits(modelType);

	// NOTE: not type-sorted
	for (const auto& unit : tmpOpaqueUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawOpaqueAIUnit(unit);
	}
}

void CUnitDrawerLegacy::DrawAlphaAIUnits(int modelType) const
{
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpAlphaUnits = unitDrawerData->GetTempAlphaDrawUnits(modelType);

	// NOTE: not type-sorted
	for (const auto& unit : tmpAlphaUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawAlphaAIUnit(unit);
		DrawAlphaAIUnitBorder(unit);
	}
}

void CUnitDrawerLegacy::DrawGhostedBuildings(int modelType) const
{
	const auto& deadGhostedBuildings = unitDrawerData->GetDeadGhostBuildings(gu->myAllyTeam, modelType);
	const auto& liveGhostedBuildings = unitDrawerData->GetLiveGhostBuildings(gu->myAllyTeam, modelType);

	glColor4f(0.6f, 0.6f, 0.6f, alphaValues.y);

	// buildings that died while ghosted
	for (GhostSolidObject* dgb : deadGhostedBuildings) {
		if (camera->InView(dgb->pos, dgb->model->GetDrawRadius())) {
			glPushMatrix();
			glTranslatef3(dgb->pos);
			glRotatef(dgb->facing * 90.0f, 0, 1, 0);

			BindModelTypeTexture(modelType, dgb->model->textureType);
			SetTeamColour(dgb->team, float2(alphaValues.y, 1.0f));

			dgb->model->DrawStatic();
			glPopMatrix();
			dgb->lastDrawFrame = globalRendering->drawFrame;
		}
	}

	for (CUnit* lgb : liveGhostedBuildings) {
		DrawAlphaUnit(lgb, modelType, true);
	}
}

void CUnitDrawerLegacy::DrawOpaqueUnit(CUnit* unit, bool drawReflection, bool drawRefraction) const
{
	if (!ShouldDrawOpaqueUnit(unit, drawReflection, drawRefraction))
		return;

	// draw the unit with the default (non-Lua) material
	SetTeamColour(unit->team);
	DrawUnitTrans(unit, 0, 0, false, false);
}

void CUnitDrawerLegacy::DrawOpaqueUnitShadow(CUnit* unit) const
{
	if (ShouldDrawOpaqueUnitShadow(unit))
		DrawUnitTrans(unit, 0, 0, false, false);
}

void CUnitDrawerLegacy::DrawAlphaUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass) const
{
	if (!unitDrawerData->IsAlpha(unit))
		return;

	if (!camera->InView(unit->drawMidPos, unit->GetDrawRadius()))
		return;

	if (LuaObjectDrawer::AddAlphaMaterialObject(unit, LUAOBJ_UNIT))
		return;

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	if (drawGhostBuildingsPass) {
		// check for decoy models
		const UnitDef* decoyDef = unit->unitDef->decoyDef;
		const S3DModel* model = nullptr;

		if (decoyDef == nullptr) {
			model = unit->model;
		}
		else {
			model = decoyDef->LoadModel();
		}

		// FIXME: needs a second pass
		if (model->type != modelType)
			return;

		// ghosted enemy units
		if (losStatus & LOS_CONTRADAR) {
			glColor4f(0.9f, 0.9f, 0.9f, alphaValues.z);
		}
		else {
			glColor4f(0.6f, 0.6f, 0.6f, alphaValues.y);
		}

		glPushMatrix();
		glTranslatef3(unit->drawPos);
		glRotatef(unit->buildFacing * 90.0f, 0, 1, 0);

		// the units in liveGhostedBuildings[modelType] are not
		// sorted by textureType, but we cannot merge them with
		// alphaModelRenderers[modelType] either since they are
		// not actually cloaked
		BindModelTypeTexture(modelType, model->textureType);

		SetTeamColour(unit->team, float2((losStatus & LOS_CONTRADAR) ? alphaValues.z : alphaValues.y, 1.0f));
		model->DrawStatic();
		glPopMatrix();

		glColor4f(1.0f, 1.0f, 1.0f, alphaValues.x);
		return;
	}

	if (unit->isIcon)
		return;

	if ((losStatus & LOS_INLOS) || gu->spectatingFullView) {
		SetTeamColour(unit->team, float2(alphaValues.x, 1.0f));
		DrawUnitTrans(unit, 0, 0, false, false);
	}
}

void CUnitDrawerLegacy::DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	glPushMatrix();
	glTranslatef3(unit.pos);
	glRotatef(unit.rotation * math::RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

	const UnitDef* def = unit.unitDef;
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	BindModelTypeTexture(mdl->type, mdl->textureType);
	SetTeamColour(unit.team);
	mdl->DrawStatic();

	glPopMatrix();
}

void CUnitDrawerLegacy::DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	glPushMatrix();
	glTranslatef3(unit.pos);
	glRotatef(unit.rotation * math::RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

	const UnitDef* def = unit.unitDef;
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	BindModelTypeTexture(mdl->type, mdl->textureType);
	SetTeamColour(unit.team, float2(alphaValues.x, 1.0f));
	mdl->DrawStatic();

	glPopMatrix();
}

void CUnitDrawerLegacy::DrawAlphaAIUnitBorder(const CUnitDrawerData::TempDrawUnit& unit) const
{
	if (!unit.drawBorder)
		return;

	SetTeamColour(unit.team, float2(alphaValues.w, 1.0f));

	const BuildInfo buildInfo(unit.unitDef, unit.pos, unit.facing);
	const float3 buildPos = CGameHelper::Pos2BuildPos(buildInfo, false);

	const float xsize = buildInfo.GetXSize() * (SQUARE_SIZE >> 1);
	const float zsize = buildInfo.GetZSize() * (SQUARE_SIZE >> 1);

	glColor4f(0.2f, 1, 0.2f, alphaValues.w);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_STRIP);
	glVertexf3(buildPos + float3(xsize, 1.0f, zsize));
	glVertexf3(buildPos + float3(-xsize, 1.0f, zsize));
	glVertexf3(buildPos + float3(-xsize, 1.0f, -zsize));
	glVertexf3(buildPos + float3(xsize, 1.0f, -zsize));
	glVertexf3(buildPos + float3(xsize, 1.0f, zsize));
	glEnd();
	glColor4f(1.0f, 1.0f, 1.0f, alphaValues.x);
	glEnable(GL_TEXTURE_2D);
}

void CUnitDrawerLegacy::DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const
{
	if (unit->noMinimap)
		return;
	if (unit->myIcon == nullptr)
		return;
	if (unit->IsInVoid())
		return;

	const unsigned char defaultColor[4] = { 255, 255, 255, 255 };
	const unsigned char* color = &defaultColor[0];

	if (!unit->isSelected) {
		if (minimap->UseSimpleColors()) {
			if (unit->team == gu->myTeam) {
				color = minimap->GetMyTeamIconColor();
			}
			else if (teamHandler.Ally(gu->myAllyTeam, unit->allyteam)) {
				color = minimap->GetAllyTeamIconColor();
			}
			else {
				color = minimap->GetEnemyTeamIconColor();
			}
		}
		else {
			color = teamHandler.Team(unit->team)->color;
		}
	}

	const float iconScale = CUnitDrawerHelper::GetUnitIconScale(unit);
	const float3& iconPos = (!gu->spectatingFullView) ?
		unit->GetObjDrawErrorPos(gu->myAllyTeam) :
		unit->GetObjDrawMidPos();

	const float iconSizeX = (iconScale * minimap->GetUnitSizeX());
	const float iconSizeY = (iconScale * minimap->GetUnitSizeY());

	const float x0 = iconPos.x - iconSizeX;
	const float x1 = iconPos.x + iconSizeX;
	const float y0 = iconPos.z - iconSizeY;
	const float y1 = iconPos.z + iconSizeY;

	unit->myIcon->DrawArray(va, x0, y0, x1, y1, color);
}

void CUnitDrawerLegacy::DrawIcon(CUnit* unit, bool useDefaultIcon)
{
	// iconUnits should not never contain void-space units, see UpdateUnitIconState
	assert(!unit->IsInVoid());

	// If the icon is to be drawn as a radar blip, we want to get the default icon.
	const icon::CIconData* iconData = nullptr;

	if (useDefaultIcon) {
		iconData = icon::iconHandler.GetDefaultIconData();
	}
	else {
		iconData = unit->unitDef->iconType.GetIconData();
	}

	// drawMidPos is auto-calculated now; can wobble on its own as pieces move
	float3 pos = (!gu->spectatingFullView) ?
		unit->GetObjDrawErrorPos(gu->myAllyTeam) :
		unit->GetObjDrawMidPos();

	// make sure icon is above ground (needed before we calculate scale below)
	const float h = CGround::GetHeightReal(pos.x, pos.z, false);

	pos.y = std::max(pos.y, h);

	// Calculate the icon size. It scales with:
	//  * The square root of the camera distance.
	//  * The mod defined 'iconSize' (which acts a multiplier).
	//  * The unit radius, depending on whether the mod defined 'radiusadjust' is true or false.
	const float dist = std::min(8000.0f, fastmath::sqrt_builtin(camera->GetPos().SqDistance(pos)));
	const float iconScaleDist = 0.4f * fastmath::sqrt_builtin(dist); // makes far icons bigger
	float scale = iconData->GetSize() * iconScaleDist;

	if (iconData->GetRadiusAdjust() && !useDefaultIcon)
		scale *= (unit->radius / iconData->GetRadiusScale());

	// make sure icon is not partly under ground
	pos.y = std::max(pos.y, h + (unit->iconRadius = scale));

	// use white for selected units
	const uint8_t* colors[] = { teamHandler.Team(unit->team)->color, color4::white };
	const uint8_t* color = colors[unit->isSelected];

	glColor3ubv(color);

	// calculate the vertices
	const float3 dy = camera->GetUp() * scale;
	const float3 dx = camera->GetRight() * scale;
	const float3 vn = pos - dx;
	const float3 vp = pos + dx;
	const float3 vnn = vn - dy;
	const float3 vpn = vp - dy;
	const float3 vnp = vn + dy;
	const float3 vpp = vp + dy;

	// Draw the icon.
	iconData->Draw(vnn, vpn, vnp, vpp);
}

void CUnitDrawerLegacy::DrawIconScreenArray(const CUnit* unit, const icon::CIconData* icon, bool useDefaultIcon, const float dist, CVertexArray* va) const
{
	// iconUnits should not never contain void-space units, see UpdateUnitIconState
	assert(!unit->IsInVoid());

	// drawMidPos is auto-calculated now; can wobble on its own as pieces move
	float3 pos = (!gu->spectatingFullView) ?
		unit->GetObjDrawErrorPos(gu->myAllyTeam) :
		unit->GetObjDrawMidPos();

	pos = camera->CalcWindowCoordinates(pos);
	if (pos.z < 0)
		return;

	// use white for selected units
	const uint8_t* srcColor = unit->isSelected ? color4::white : teamHandler.Team(unit->team)->color;
	uint8_t color[4] = { srcColor[0], srcColor[1], srcColor[2], 255 };

	float unitRadiusMult = icon->GetSize();
	if (icon->GetRadiusAdjust() && !useDefaultIcon)
		unitRadiusMult *= (unit->radius / icon->GetRadiusScale());
	unitRadiusMult = (unitRadiusMult - 1) * 0.75 + 1;

	// fade icons away in high zoom in levels
	if (!unit->isIcon)
		if (dist / unitRadiusMult < unitDrawerData->iconFadeVanish)
			return;
		else if (unitDrawerData->iconFadeVanish < unitDrawerData->iconFadeStart && dist / unitRadiusMult < unitDrawerData->iconFadeStart)
			// alpha range [64, 255], since icons is unrecognisable with alpha < 64
			color[3] = 64 + 191.0f * (dist / unitRadiusMult - unitDrawerData->iconFadeVanish) / (unitDrawerData->iconFadeStart - unitDrawerData->iconFadeVanish);

	// calculate the vertices
	const float offset = unitDrawerData->iconSizeBase / 2.0f * unitRadiusMult;

	const float x0 = (pos.x - offset) / globalRendering->viewSizeX;
	const float y0 = (pos.y + offset) / globalRendering->viewSizeY;
	const float x1 = (pos.x + offset) / globalRendering->viewSizeX;
	const float y1 = (pos.y - offset) / globalRendering->viewSizeY;

	if (x1 < 0 || x0 > 1 || y0 < 0 || y1 > 1)
		return; // don't try to draw outside the screen

	// Draw the icon.
	icon->DrawArray(va, x0, y0, x1, y1, color);
}


void CUnitDrawerLegacy::DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const
{
	const float3 stageBounds = { 0.0f, unit->model->CalcDrawHeight(), unit->buildProgress };

	// draw-height defaults to maxs.y - mins.y, but can be overridden for non-3DO models
	// the default value derives from the model vertices and makes more sense to use here
	//
	// Both clip planes move up. Clip plane 0 is the upper bound of the model,
	// clip plane 1 is the lower bound. In other words, clip plane 0 makes the
	// wireframe/flat color/texture appear, and clip plane 1 then erases the
	// wireframe/flat color later on.
	const double upperPlanes[BuildStages::BUILDSTAGE_CNT][4] = {
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const double lowerPlanes[BuildStages::BUILDSTAGE_CNT][4] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 3.0f  - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                  (0.0f)},
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_CURRENT_BIT);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	if (stageBounds.z > 0.0f / 3.0f) {
		// wireframe, unconditional
		DrawModelWireBuildStageShadow(unit, upperPlanes[BUILDSTAGE_WIRE], lowerPlanes[BUILDSTAGE_WIRE], noLuaCall, globalRendering->amdHacks);
	}

	if (stageBounds.z > 1.0f / 3.0f) {
		// flat-colored, conditional
		DrawModelFlatBuildStageShadow(unit, upperPlanes[BUILDSTAGE_FLAT], lowerPlanes[BUILDSTAGE_FLAT], noLuaCall);
	}

	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE0);

	if (stageBounds.z > 2.0f / 3.0f) {
		// fully-shaded, conditional
		DrawModelFillBuildStageShadow(unit, upperPlanes[BUILDSTAGE_FILL], lowerPlanes[BUILDSTAGE_FILL], noLuaCall);
	}

	glPopAttrib();
}

void CUnitDrawerLegacy::DrawModelWireBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall, bool amdHack) const
{
	if (amdHack) {
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
	} else {
		glPushMatrix();
		glLoadIdentity();
		glClipPlane(GL_CLIP_PLANE0, upperPlane);
		glClipPlane(GL_CLIP_PLANE1, lowerPlane);
		glPopMatrix();
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	DrawUnitModel(unit, noLuaCall);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (amdHack) {
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
	}
}

void CUnitDrawerLegacy::DrawModelFlatBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	glPushMatrix();
	glLoadIdentity();
	glClipPlane(GL_CLIP_PLANE0, upperPlane);
	glClipPlane(GL_CLIP_PLANE1, lowerPlane);
	glPopMatrix();

	DrawUnitModel(unit, noLuaCall);
}

void CUnitDrawerLegacy::DrawModelFillBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	DrawUnitModel(unit, noLuaCall);
}

void CUnitDrawerLegacy::DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const
{
	const S3DModel* model = unit->model;
	const    CTeam* team = teamHandler.Team(unit->team);
	const   SColor  color = team->color;

	const float wireColorMult = std::fabs(128.0f - ((gs->frameNum * 4) & 255)) / 255.0f + 0.5f;
	const float flatColorMult = 1.5f - wireColorMult;

	const float3 frameColors[2] = { unit->unitDef->nanoColor, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f} };
	const float3 stageColors[2] = { frameColors[globalRendering->teamNanospray], frameColors[globalRendering->teamNanospray] };
	const float3 stageBounds = { 0.0f, model->CalcDrawHeight(), unit->buildProgress };

	// draw-height defaults to maxs.y - mins.y, but can be overridden for non-3DO models
	// the default value derives from the model vertices and makes more sense to use here
	//
	// Both clip planes move up. Clip plane 0 is the upper bound of the model,
	// clip plane 1 is the lower bound. In other words, clip plane 0 makes the
	// wireframe/flat color/texture appear, and clip plane 1 then erases the
	// wireframe/flat color later on.
	const double upperPlanes[4][4] = {
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const double lowerPlanes[4][4] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                  (0.0f)},
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_CURRENT_BIT);
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	if (stageBounds.z > 0.0f / 3.0f) {
		// wireframe, unconditional
		SetNanoColor(float4(stageColors[0] * wireColorMult, 1.0f));
		DrawModelWireBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_WIRE], lowerPlanes[BUILDSTAGE_WIRE], noLuaCall, globalRendering->amdHacks);
	}

	if (stageBounds.z > 1.0f / 3.0f) {
		// flat-colored, conditional
		SetNanoColor(float4(stageColors[1] * flatColorMult, 1.0f));
		DrawModelFlatBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_WIRE], lowerPlanes[BUILDSTAGE_WIRE], noLuaCall);
	}

	glDisable(GL_CLIP_PLANE1);

	if (stageBounds.z > 2.0f / 3.0f) {
		// fully-shaded, conditional
		SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f)); // turn off
		DrawModelFillBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_FILL], lowerPlanes[BUILDSTAGE_FILL], noLuaCall, globalRendering->amdHacks);
	}

	glDisable(GL_CLIP_PLANE0);
	glPopAttrib();
}

void CUnitDrawerLegacy::DrawModelWireBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall, bool amdHack) const
{
	if (amdHack) {
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
	} else {
		glClipPlane(GL_CLIP_PLANE0, upperPlane);
		glClipPlane(GL_CLIP_PLANE1, lowerPlane);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	DrawUnitModel(unit, noLuaCall);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (amdHack) {
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
	}
}

void CUnitDrawerLegacy::DrawModelFlatBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	glClipPlane(GL_CLIP_PLANE0, upperPlane);
	glClipPlane(GL_CLIP_PLANE1, lowerPlane);

	DrawUnitModel(unit, noLuaCall);
}

void CUnitDrawerLegacy::DrawModelFillBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall, bool amdHack) const
{
	if (amdHack)
		glDisable(GL_CLIP_PLANE0);
	else
		glClipPlane(GL_CLIP_PLANE0, upperPlane);

	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	DrawUnitModel(unit, noLuaCall);
	glDisable(GL_POLYGON_OFFSET_FILL);
}

void CUnitDrawerLegacy::PushIndividualOpaqueState(const CUnit* unit, bool deferredPass) const { PushIndividualOpaqueState(unit->model, unit->team, deferredPass); }
void CUnitDrawerLegacy::PushIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass) const
{
	// these are not handled by Setup*Drawing but CGame
	// easier to assume they no longer have the correct
	// values at this point
	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	SetupOpaqueDrawing(deferredPass);
	PushModelRenderState(model);
	SetTeamColour(teamID);
}

void CUnitDrawerLegacy::PushIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const
{
	SetupAlphaDrawing(deferredPass);
	PushModelRenderState(model);
	SetTeamColour(teamID, float2(alphaValues.x, 1.0f));
}

void CUnitDrawerLegacy::PopIndividualOpaqueState(const CUnit* unit, bool deferredPass) const { PopIndividualOpaqueState(unit->model, unit->team, deferredPass); }
void CUnitDrawerLegacy::PopIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass) const
{
	PopModelRenderState(model);
	ResetOpaqueDrawing(deferredPass);

	glPopAttrib();
}

void CUnitDrawerLegacy::PopIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const
{
	PopModelRenderState(model);
	ResetAlphaDrawing(deferredPass);
}

void CUnitDrawerLegacy::DrawIndividual(const CUnit* unit, bool noLuaCall) const
{
	if (LuaObjectDrawer::DrawSingleObject(unit, LUAOBJ_UNIT /*, noLuaCall*/))
		return;

	// set the full default state
	PushIndividualOpaqueState(unit, false);
	DrawUnitTrans(unit, 0, 0, false, noLuaCall);
	PopIndividualOpaqueState(unit, false);
}

void CUnitDrawerLegacy::DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const
{
	if (LuaObjectDrawer::DrawSingleObjectNoTrans(unit, LUAOBJ_UNIT /*, noLuaCall*/))
		return;

	PushIndividualOpaqueState(unit, false);
	DrawUnitNoTrans(unit, 0, 0, false, noLuaCall);
	PopIndividualOpaqueState(unit, false);
}

void CUnitDrawerLegacy::DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen) const
{
	const S3DModel* model = objectDef->LoadModel();

	if (model == nullptr)
		return;

	if (!rawState) {
		if (!CUnitDrawerHelper::DIDCheckMatrixMode(GL_MODELVIEW))
			return;

		// teamID validity is checked by SetTeamColour
		PushIndividualOpaqueState(model, teamID, false);

		// NOTE:
		//   unlike DrawIndividual(...) the model transform is
		//   always provided by Lua, not taken from the object
		//   (which does not exist here) so we must restore it
		//   (by undoing the UnitDrawerState MVP setup)
		//
		//   assumes the Lua transform includes a LoadIdentity!
		CUnitDrawerHelper::DIDResetPrevProjection(toScreen);
		CUnitDrawerHelper::DIDResetPrevModelView();
	}

	model->DrawStatic();

	if (!rawState) {
		PopIndividualOpaqueState(model, teamID, false);
	}
}

void CUnitDrawerLegacy::DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen) const
{
	const S3DModel* model = objectDef->LoadModel();

	if (model == nullptr)
		return;

	if (!rawState) {
		if (!CUnitDrawerHelper::DIDCheckMatrixMode(GL_MODELVIEW))
			return;

		PushIndividualAlphaState(model, teamID, false);

		CUnitDrawerHelper::DIDResetPrevProjection(toScreen);
		CUnitDrawerHelper::DIDResetPrevModelView();
	}

	model->DrawStatic();

	if (!rawState) {
		PopIndividualAlphaState(model, teamID, false);
	}
}


bool CUnitDrawerLegacy::ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands) const
{
	//TODO: make this a lua callin!
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	CFeature* feature = nullptr;

	std::vector<float3> buildableSquares; // buildable squares
	std::vector<float3> featureSquares; // occupied squares
	std::vector<float3> illegalSquares; // non-buildable squares

	const float3& pos = buildInfo.pos;
	const int x1 = pos.x - (buildInfo.GetXSize() * 0.5f * SQUARE_SIZE);
	const int x2 = x1 + (buildInfo.GetXSize() * SQUARE_SIZE);
	const int z1 = pos.z - (buildInfo.GetZSize() * 0.5f * SQUARE_SIZE);
	const int z2 = z1 + (buildInfo.GetZSize() * SQUARE_SIZE);
	const float h = CGameHelper::GetBuildHeight(pos, buildInfo.def, false);

	const bool canBuild = !!CGameHelper::TestUnitBuildSquare(
		buildInfo,
		feature,
		-1,
		false,
		&buildableSquares,
		&featureSquares,
		&illegalSquares,
		&commands
	);

	if (canBuild) {
		glColor4f(0.0f, 0.9f, 0.0f, 0.7f);
	}
	else {
		glColor4f(0.9f, 0.8f, 0.0f, 0.7f);
	}

	CVertexArray* va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(buildableSquares.size() * 4, 0, VA_SIZE_0);

	for (uint32_t i = 0; i < buildableSquares.size(); i++) {
		va->AddVertexQ0(buildableSquares[i]);
		va->AddVertexQ0(buildableSquares[i] + float3(SQUARE_SIZE, 0, 0));
		va->AddVertexQ0(buildableSquares[i] + float3(SQUARE_SIZE, 0, SQUARE_SIZE));
		va->AddVertexQ0(buildableSquares[i] + float3(0, 0, SQUARE_SIZE));
	}
	va->DrawArray0(GL_QUADS);


	glColor4f(0.9f, 0.8f, 0.0f, 0.7f);
	va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(featureSquares.size() * 4, 0, VA_SIZE_0);

	for (uint32_t i = 0; i < featureSquares.size(); i++) {
		va->AddVertexQ0(featureSquares[i]);
		va->AddVertexQ0(featureSquares[i] + float3(SQUARE_SIZE, 0, 0));
		va->AddVertexQ0(featureSquares[i] + float3(SQUARE_SIZE, 0, SQUARE_SIZE));
		va->AddVertexQ0(featureSquares[i] + float3(0, 0, SQUARE_SIZE));
	}
	va->DrawArray0(GL_QUADS);


	glColor4f(0.9f, 0.0f, 0.0f, 0.7f);
	va = GetVertexArray();
	va->Initialize();
	va->EnlargeArrays(illegalSquares.size() * 4, 0, VA_SIZE_0);

	for (uint32_t i = 0; i < illegalSquares.size(); i++) {
		va->AddVertexQ0(illegalSquares[i]);
		va->AddVertexQ0(illegalSquares[i] + float3(SQUARE_SIZE, 0, 0));
		va->AddVertexQ0(illegalSquares[i] + float3(SQUARE_SIZE, 0, SQUARE_SIZE));
		va->AddVertexQ0(illegalSquares[i] + float3(0, 0, SQUARE_SIZE));
	}
	va->DrawArray0(GL_QUADS);


	if (h < 0.0f) {
		const unsigned char s[4] = { 0,   0, 255, 128 }; // start color
		const unsigned char e[4] = { 0, 128, 255, 255 }; // end color

		va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(8, 0, VA_SIZE_C);
		va->AddVertexQC(float3(x1, h, z1), s); va->AddVertexQC(float3(x1, 0.f, z1), e);
		va->AddVertexQC(float3(x1, h, z2), s); va->AddVertexQC(float3(x1, 0.f, z2), e);
		va->AddVertexQC(float3(x2, h, z2), s); va->AddVertexQC(float3(x2, 0.f, z2), e);
		va->AddVertexQC(float3(x2, h, z1), s); va->AddVertexQC(float3(x2, 0.f, z1), e);
		va->DrawArrayC(GL_LINES);

		va = GetVertexArray();
		va->Initialize();
		va->AddVertexQC(float3(x1, 0.0f, z1), e);
		va->AddVertexQC(float3(x1, 0.0f, z2), e);
		va->AddVertexQC(float3(x2, 0.0f, z2), e);
		va->AddVertexQC(float3(x2, 0.0f, z1), e);
		va->DrawArrayC(GL_LINE_LOOP);
	}

	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	// glDisable(GL_BLEND);

	return canBuild;
}

/***********************************************************************/

bool CUnitDrawerFFP::SetTeamColour(int team, const float2 alpha) const
{
	if (!CUnitDrawer::SetTeamColour(team, alpha))
		return false;

	// non-shader case via texture combiners
	const float4 m = { 1.0f, 1.0f, 1.0f, alpha.x };

	glActiveTexture(GL_TEXTURE0);
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, std::move(CUnitDrawerHelper::GetTeamColor(team, alpha.x)));
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &m.x);

	return true;
}

void CUnitDrawerFFP::Enable(bool deferredPass, bool alphaPass) const
{
	glEnable(GL_LIGHTING);
	// only for the advshading=0 case
	glLightfv(GL_LIGHT1, GL_POSITION, sky->GetLight()->GetLightDir());
	glLightfv(GL_LIGHT1, GL_AMBIENT, sunLighting->modelAmbientColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, sunLighting->modelDiffuseColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, sunLighting->modelSpecularColor);
	glEnable(GL_LIGHT1);

	CUnitDrawerFFP::SetupBasicS3OTexture1();
	CUnitDrawerFFP::SetupBasicS3OTexture0();

	const float4 color = { 1.0f, 1.0f, 1.0, mix(1.0f, alphaValues.x, (1.0f * alphaPass)) };

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &color.x);
	glColor4fv(&color.x);

	CUnitDrawerHelper::PushTransform(camera);
}

void CUnitDrawerFFP::Disable(bool deferredPass) const
{
	CUnitDrawerHelper::PopTransform();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT1);

	CUnitDrawerFFP::CleanupBasicS3OTexture1();
	CUnitDrawerFFP::CleanupBasicS3OTexture0();
}

void CUnitDrawerFFP::SetNanoColor(const float4& color) const
{
	if (color.a > 0.0f) {
		DisableTextures();
		glColorf4(color);
	}
	else {
		EnableTextures();
		glColorf3(OnesVector);
	}
}

void CUnitDrawerFFP::EnableTextures() const
{
	glEnable(GL_LIGHTING);
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
}

void CUnitDrawerFFP::DisableTextures() const
{
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
}


/**
 * Set up the texture environment in texture unit 0
 * to give an S3O texture its team-colour.
 *
 * Also:
 * - call SetBasicTeamColour to set the team colour to transform to.
 * - Replace the output alpha channel. If not, only the team-coloured bits will show, if that. Or something.
 */
void CUnitDrawerFFP::SetupBasicS3OTexture0()
{
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	// RGB = Texture * (1 - Alpha) + Teamcolor * Alpha
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);

	// ALPHA = Ignore
}

/**
 * This sets the first texture unit to GL_MODULATE the colours from the
 * first texture unit with the current glColor.
 *
 * Normal S3O drawing sets the color to full white; translucencies
 * use this setup to 'tint' the drawn model.
 *
 * - Leaves glActivateTextureARB at the first unit.
 * - This doesn't tinker with the output alpha, either.
 */
void CUnitDrawerFFP::SetupBasicS3OTexture1()
{
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	// RGB = Primary Color * Previous
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);

	// ALPHA = Current alpha * Alpha mask
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
}

void CUnitDrawerFFP::CleanupBasicS3OTexture1()
{
	// reset texture1 state
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

void CUnitDrawerFFP::CleanupBasicS3OTexture0()
{
	// reset texture0 state
	glActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_CONSTANT_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

CUnitDrawerARB::CUnitDrawerARB()
{
	if (!CanEnable())
		return;

	// if GLEW_NV_vertex_program2 is supported, transparent objects are clipped against GL_CLIP_PLANE3
	static const char* vertProgNamesARB[2] = { "ARB/units3o.vp", "ARB/units3o2.vp" };
	static const char* fragProgNamesARB[2] = { "ARB/units3o.fp", "ARB/units3o_shadow.fp" };

	#define sh shaderHandler
	modelShaders[MODEL_SHADER_NOSHADOW_STANDARD] = sh->CreateProgramObject("[UnitDrawer]", "S3OShaderDefARB", true);
	modelShaders[MODEL_SHADER_NOSHADOW_STANDARD]->AttachShaderObject(sh->CreateShaderObject(vertProgNamesARB[GLEW_NV_vertex_program2], "", GL_VERTEX_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_NOSHADOW_STANDARD]->AttachShaderObject(sh->CreateShaderObject(fragProgNamesARB[0], "", GL_FRAGMENT_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_NOSHADOW_STANDARD]->Link();

	modelShaders[MODEL_SHADER_SHADOWED_STANDARD] = sh->CreateProgramObject("[UnitDrawer]", "S3OShaderAdvARB", true);
	modelShaders[MODEL_SHADER_SHADOWED_STANDARD]->AttachShaderObject(sh->CreateShaderObject(vertProgNamesARB[GLEW_NV_vertex_program2], "", GL_VERTEX_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_SHADOWED_STANDARD]->AttachShaderObject(sh->CreateShaderObject(fragProgNamesARB[1], "", GL_FRAGMENT_PROGRAM_ARB));
	modelShaders[MODEL_SHADER_SHADOWED_STANDARD]->Link();

	modelShaders[MODEL_SHADER_NOSHADOW_DEFERRED] = nullptr; //cannot draw deferred
	modelShaders[MODEL_SHADER_SHADOWED_DEFERRED] = nullptr;

	#undef sh

	SetActiveShader(shadowHandler.ShadowsLoaded(), false);
}

CUnitDrawerARB::~CUnitDrawerARB()
{
	modelShaders.fill(nullptr);
	shaderHandler->ReleaseProgramObjects("[UnitDrawer]");
}

bool CUnitDrawerARB::CanEnable() const { return globalRendering->haveARB && UseAdvShading(); }

bool CUnitDrawerARB::SetTeamColour(int team, const float2 alpha) const
{
	if (!CUnitDrawer::SetTeamColour(team, alpha))
		return false;

	// NOTE:
	//   both UnitDrawer::DrawAlphaPass and FeatureDrawer::DrawAlphaPass
	//   disable advShading in case of ARB, so in that case we should end
	//   up in StateFFP::SetTeamColor
	assert(modelShader != nullptr);
	assert(modelShader->IsBound());

	modelShader->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
	modelShader->SetUniform4fv(14, std::move(CUnitDrawerHelper::GetTeamColor(team, alpha.x)));

	return true;
}

void CUnitDrawerARB::Enable(bool deferredPass, bool alphaPass) const
{
	// body of former EnableCommon();
	CUnitDrawerHelper::PushTransform(camera);
	CUnitDrawerHelper::EnableTexturesCommon();

	SetActiveShader(shadowHandler.ShadowsLoaded(), /*deferredPass*/ false);
	assert(modelShader != nullptr);
	modelShader->Enable();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// end of EnableCommon();

	modelShader->SetUniformTarget(GL_VERTEX_PROGRAM_ARB);
	modelShader->SetUniform4fv(10, &sky->GetLight()->GetLightDir().x);
	modelShader->SetUniform4f(11, sunLighting->modelDiffuseColor.x, sunLighting->modelDiffuseColor.y, sunLighting->modelDiffuseColor.z, 0.0f);
	modelShader->SetUniform4f(12, sunLighting->modelAmbientColor.x, sunLighting->modelAmbientColor.y, sunLighting->modelAmbientColor.z, 1.0f); //!
	modelShader->SetUniform4f(13, camera->GetPos().x, camera->GetPos().y, camera->GetPos().z, 0.0f);
	modelShader->SetUniformTarget(GL_FRAGMENT_PROGRAM_ARB);
	modelShader->SetUniform4f(10, 0.0f, 0.0f, 0.0f, sunLighting->modelShadowDensity);
	modelShader->SetUniform4f(11, sunLighting->modelAmbientColor.x, sunLighting->modelAmbientColor.y, sunLighting->modelAmbientColor.z, 1.0f);

	glMatrixMode(GL_MATRIX0_ARB);
	glLoadMatrixf(shadowHandler.GetShadowMatrixRaw());
	glMatrixMode(GL_MODELVIEW);
}

void CUnitDrawerARB::Disable(bool deferredPass) const
{
	assert(modelShader != nullptr);

	modelShader->Disable();
	SetActiveShader(shadowHandler.ShadowsLoaded(), /*deferredPass*/ false);

	CUnitDrawerHelper::DisableTexturesCommon();
	CUnitDrawerHelper::PopTransform();
}

void CUnitDrawerARB::SetNanoColor(const float4& color) const
{
	if (color.a > 0.0f) {
		glColorf4(color);
	}
	else {
		glColorf3(OnesVector);
	}
}

void CUnitDrawerARB::EnableTextures() const { CUnitDrawerHelper::EnableTexturesCommon(); }
void CUnitDrawerARB::DisableTextures() const { CUnitDrawerHelper::DisableTexturesCommon(); }

CUnitDrawerGLSL::CUnitDrawerGLSL()
{
	if (!CanEnable())
		return;

	#define sh shaderHandler

	const GL::LightHandler* lightHandler = CUnitDrawer::GetLightHandler();
	static const std::string shaderNames[MODEL_SHADER_COUNT] = {
		"ModelShaderGLSL-NoShadowStandard",
		"ModelShaderGLSL-ShadowedStandard",
		"ModelShaderGLSL-NoShadowDeferred",
		"ModelShaderGLSL-ShadowedDeferred",
	};
	const std::string extraDefs =
		("#define BASE_DYNAMIC_MODEL_LIGHT " + IntToString(lightHandler->GetBaseLight()) + "\n") +
		("#define MAX_DYNAMIC_MODEL_LIGHTS " + IntToString(lightHandler->GetMaxLights()) + "\n");

	for (uint32_t n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n] = sh->CreateProgramObject("[UnitDrawer]", shaderNames[n], false);
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelVertProg.glsl", extraDefs, GL_VERTEX_SHADER));
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelFragProg.glsl", extraDefs, GL_FRAGMENT_SHADER));

		modelShaders[n]->SetFlag("USE_SHADOWS", int((n & 1) == 1));
		modelShaders[n]->SetFlag("DEFERRED_MODE", int(n >= MODEL_SHADER_NOSHADOW_DEFERRED));
		modelShaders[n]->SetFlag("GBUFFER_NORMTEX_IDX", GL::GeometryBuffer::ATTACHMENT_NORMTEX);
		modelShaders[n]->SetFlag("GBUFFER_DIFFTEX_IDX", GL::GeometryBuffer::ATTACHMENT_DIFFTEX);
		modelShaders[n]->SetFlag("GBUFFER_SPECTEX_IDX", GL::GeometryBuffer::ATTACHMENT_SPECTEX);
		modelShaders[n]->SetFlag("GBUFFER_EMITTEX_IDX", GL::GeometryBuffer::ATTACHMENT_EMITTEX);
		modelShaders[n]->SetFlag("GBUFFER_MISCTEX_IDX", GL::GeometryBuffer::ATTACHMENT_MISCTEX);
		modelShaders[n]->SetFlag("GBUFFER_ZVALTEX_IDX", GL::GeometryBuffer::ATTACHMENT_ZVALTEX);

		modelShaders[n]->Link();
		modelShaders[n]->SetUniformLocation("diffuseTex");        // idx  0 (t1: diffuse + team-color)
		modelShaders[n]->SetUniformLocation("shadingTex");        // idx  1 (t2: spec/refl + self-illum)
		modelShaders[n]->SetUniformLocation("shadowTex");         // idx  2
		modelShaders[n]->SetUniformLocation("reflectTex");        // idx  3 (cube)
		modelShaders[n]->SetUniformLocation("specularTex");       // idx  4 (cube)
		modelShaders[n]->SetUniformLocation("sunDir");            // idx  5
		modelShaders[n]->SetUniformLocation("cameraPos");         // idx  6
		modelShaders[n]->SetUniformLocation("cameraMat");         // idx  7
		modelShaders[n]->SetUniformLocation("cameraMatInv");      // idx  8
		modelShaders[n]->SetUniformLocation("teamColor");         // idx  9
		modelShaders[n]->SetUniformLocation("nanoColor");         // idx 10
		modelShaders[n]->SetUniformLocation("sunAmbient");        // idx 11
		modelShaders[n]->SetUniformLocation("sunDiffuse");        // idx 12
		modelShaders[n]->SetUniformLocation("shadowDensity");     // idx 13
		modelShaders[n]->SetUniformLocation("shadowMatrix");      // idx 14
		modelShaders[n]->SetUniformLocation("shadowParams");      // idx 15
		// modelShaders[n]->SetUniformLocation("alphaPass");         // idx 16

		modelShaders[n]->Enable();
		modelShaders[n]->SetUniform1i(0, 0); // diffuseTex  (idx 0, texunit 0)
		modelShaders[n]->SetUniform1i(1, 1); // shadingTex  (idx 1, texunit 1)
		modelShaders[n]->SetUniform1i(2, 2); // shadowTex   (idx 2, texunit 2)
		modelShaders[n]->SetUniform1i(3, 3); // reflectTex  (idx 3, texunit 3)
		modelShaders[n]->SetUniform1i(4, 4); // specularTex (idx 4, texunit 4)
		modelShaders[n]->SetUniform3fv(5, &sky->GetLight()->GetLightDir().x);
		modelShaders[n]->SetUniform3fv(6, &camera->GetPos()[0]);
		modelShaders[n]->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
		modelShaders[n]->SetUniformMatrix4fv(8, false, camera->GetViewMatrixInverse());
		modelShaders[n]->SetUniform4f(9, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform4f(10, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->SetUniform3fv(11, &sunLighting->modelAmbientColor[0]);
		modelShaders[n]->SetUniform3fv(12, &sunLighting->modelDiffuseColor[0]);
		modelShaders[n]->SetUniform1f(13, sunLighting->modelShadowDensity);
		modelShaders[n]->SetUniformMatrix4fv(14, false, shadowHandler.GetShadowMatrixRaw());
		modelShaders[n]->SetUniform4fv(15, &(shadowHandler.GetShadowParams().x));
		// modelShaders[n]->SetUniform1f(16, 0.0f); // alphaPass
		modelShaders[n]->Disable();
		modelShaders[n]->Validate();
	}

	// make the active shader non-NULL
	SetActiveShader(shadowHandler.ShadowsLoaded(), false);

	#undef sh

}

CUnitDrawerGLSL::~CUnitDrawerGLSL()
{
	modelShaders.fill(nullptr);
	shaderHandler->ReleaseProgramObjects("[UnitDrawer]");
}

bool CUnitDrawerGLSL::CanEnable() const { return globalRendering->haveGLSL && UseAdvShading(); }

bool CUnitDrawerGLSL::CanDrawDeferred() const { return deferredAllowed; }

bool CUnitDrawerGLSL::SetTeamColour(int team, const float2 alpha) const
{
	if (!CUnitDrawer::SetTeamColour(team, alpha))
		return false;

	assert(modelShader != nullptr);
	assert(modelShader->IsBound());

	modelShader->SetUniform4fv(9, std::move(CUnitDrawerHelper::GetTeamColor(team, alpha.x)));
	// modelShaders[MODEL_SHADER_ACTIVE]->SetUniform1f(16, alpha.y);

	return true;
}

void CUnitDrawerGLSL::Enable(bool deferredPass, bool alphaPass) const
{
	// body of former EnableCommon();
	CUnitDrawerHelper::PushTransform(camera);
	CUnitDrawerHelper::EnableTexturesCommon();

	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);
	assert(modelShader != nullptr);
	modelShader->Enable();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// end of EnableCommon();

	modelShader->SetUniform3fv(6, &camera->GetPos()[0]);
	modelShader->SetUniformMatrix4fv(7, false, camera->GetViewMatrix());
	modelShader->SetUniformMatrix4fv(8, false, camera->GetViewMatrixInverse());
	modelShader->SetUniformMatrix4fv(14, false, shadowHandler.GetShadowMatrixRaw());
	modelShader->SetUniform4fv(15, &(shadowHandler.GetShadowParams().x));

	const_cast<GL::LightHandler*>(CUnitDrawer::GetLightHandler())->Update(modelShader);
}

void CUnitDrawerGLSL::Disable(bool deferredPass) const
{
	assert(modelShader != nullptr);

	modelShader->Disable();
	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);

	CUnitDrawerHelper::DisableTexturesCommon();
	CUnitDrawerHelper::PopTransform();
}

void CUnitDrawerGLSL::SetNanoColor(const float4& color) const
{
	assert(modelShader->IsBound());
	modelShader->SetUniform4fv(10, color);
}

void CUnitDrawerGLSL::EnableTextures() const { CUnitDrawerHelper::EnableTexturesCommon(); }
void CUnitDrawerGLSL::DisableTextures() const { CUnitDrawerHelper::DisableTexturesCommon(); }



CUnitDrawerGL4::CUnitDrawerGL4()
{
	if (!CanEnable())
		return;

	#define sh shaderHandler

	const GL::LightHandler* lightHandler = CUnitDrawer::GetLightHandler();
	static const std::string shaderNames[MODEL_SHADER_COUNT] = {
		"ModelShaderGL4-NoShadowStandard",
		"ModelShaderGL4-ShadowedStandard",
		"ModelShaderGL4-NoShadowDeferred",
		"ModelShaderGL4-ShadowedDeferred",
	};

	for (uint32_t n = MODEL_SHADER_NOSHADOW_STANDARD; n <= MODEL_SHADER_SHADOWED_DEFERRED; n++) {
		modelShaders[n] = sh->CreateProgramObject("[UnitDrawer-GL4]", shaderNames[n], false);
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelVertProgGL4.glsl", "", GL_VERTEX_SHADER));
		modelShaders[n]->AttachShaderObject(sh->CreateShaderObject("GLSL/ModelFragProgGL4.glsl", "", GL_FRAGMENT_SHADER));

		modelShaders[n]->SetFlag("USE_SHADOWS", int((n & 1) == 1));
		modelShaders[n]->SetFlag("DEFERRED_MODE", int(n >= MODEL_SHADER_NOSHADOW_DEFERRED));
		modelShaders[n]->SetFlag("GBUFFER_NORMTEX_IDX", GL::GeometryBuffer::ATTACHMENT_NORMTEX);
		modelShaders[n]->SetFlag("GBUFFER_DIFFTEX_IDX", GL::GeometryBuffer::ATTACHMENT_DIFFTEX);
		modelShaders[n]->SetFlag("GBUFFER_SPECTEX_IDX", GL::GeometryBuffer::ATTACHMENT_SPECTEX);
		modelShaders[n]->SetFlag("GBUFFER_EMITTEX_IDX", GL::GeometryBuffer::ATTACHMENT_EMITTEX);
		modelShaders[n]->SetFlag("GBUFFER_MISCTEX_IDX", GL::GeometryBuffer::ATTACHMENT_MISCTEX);
		modelShaders[n]->SetFlag("GBUFFER_ZVALTEX_IDX", GL::GeometryBuffer::ATTACHMENT_ZVALTEX);

		modelShaders[n]->Link();
		//modelShaders[n]->SetUniformLocation("teamColor");         // idx  9
		//modelShaders[n]->SetUniformLocation("nanoColor");         // idx 10

		modelShaders[n]->Enable();
		//modelShaders[n]->SetUniform4f(9, 0.0f, 0.0f, 0.0f, 0.0f);
		//modelShaders[n]->SetUniform4f(10, 0.0f, 0.0f, 0.0f, 0.0f);
		modelShaders[n]->Disable();
		modelShaders[n]->Validate();
	}

	// make the active shader non-NULL
	SetActiveShader(shadowHandler.ShadowsLoaded(), false);

	#undef sh

}

CUnitDrawerGL4::~CUnitDrawerGL4()
{
	modelShaders.fill(nullptr);
	shaderHandler->ReleaseProgramObjects("[UnitDrawer-GL4]");
}

bool CUnitDrawerGL4::CanEnable() const { return globalRendering->haveGL4 && UseAdvShading(); }
bool CUnitDrawerGL4::CanDrawDeferred() const { return deferredAllowed; }

void CUnitDrawerGL4::DrawOpaqueUnitsShadow(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const
{
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		const auto& binUnits = rdrCntProxy.GetObjectBin(i);

		if (!mtModelDrawer) {
			static vector<CUnit*> renderUnits;
			renderUnits.resize(binUnits.size());

			for_mt(0, binUnits.size(), [&binUnits, this](const int i) {
				renderUnits[i] = ShouldDrawOpaqueUnitShadow(binUnits[i]) ? binUnits[i] : nullptr;
			});

			for (CUnit* unit : renderUnits) {
				if (!unit)
					continue;

				smv.AddToSubmission(unit);
			}
		}
		else {
			for (auto* unit : binUnits) {
				if (!ShouldDrawOpaqueUnitShadow(unit))
					continue;

				smv.AddToSubmission(unit);
			}
		}
		smv.Submit(GL_TRIANGLES, false);

		CUnitDrawerHelper::unitDrawerHelpers[modelType]->UnbindShadowTex(nullptr);
	}

	smv.Unbind();
}

void CUnitDrawerGL4::DrawOpaqueUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const
{
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	SetColorMultiplier();

	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		if (!mtModelDrawer) {
			for (const CUnit* unit : rdrCntProxy.GetObjectBin(i)) {
				//DrawOpaqueUnit(unit, drawReflection, drawRefraction);
				if (!ShouldDrawOpaqueUnit(unit, drawReflection, drawRefraction))
					continue;

				smv.AddToSubmission(unit);
			}
		}
		else {
			const auto& bin = rdrCntProxy.GetObjectBin(i);
			static std::vector<const CUnit*> unitList;
			unitList.resize(bin.size());
			for_mt(0, unitList.size(), [this, &bin, drawReflection, drawRefraction](int k) {
				const CUnit* unit = bin[k];
				unitList[k] = ShouldDrawOpaqueUnit(unit, drawReflection, drawRefraction) ? unit : nullptr;
				});

			for (const CUnit* unit : unitList) if (unit)
				smv.AddToSubmission(unit);
		}
		smv.Submit(GL_TRIANGLES, false);
	}

	smv.Unbind();
}

void CUnitDrawerGL4::DrawAlphaUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const
{
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	SetColorMultiplier(alphaValues.x);

	//main cloaked alpha pass
	for (uint32_t i = 0, n = rdrCntProxy.GetNumObjectBins(); i < n; i++) {
		BindModelTypeTexture(modelType, rdrCntProxy.GetObjectBinKey(i));

		const auto& binUnits = rdrCntProxy.GetObjectBin(i);

		if (!mtModelDrawer) {
			for (auto* unit : binUnits) {
				if (!ShouldDrawAlphaUnit(unit))
					continue;

				smv.AddToSubmission(unit);
			}
		}
		else {
			static vector<CUnit*> renderUnits;

			renderUnits.resize(binUnits.size());

			for_mt(0, binUnits.size(), [&binUnits, this](const int i) {
				renderUnits[i] = ShouldDrawAlphaUnit(binUnits[i]) ? binUnits[i] : nullptr;
				});

			for (CUnit* unit : renderUnits) {
				if (!unit)
					continue;

				smv.AddToSubmission(unit);
			}
		}

		smv.Submit(GL_TRIANGLES, false);
	}

	// void CGLUnitDrawer::DrawGhostedBuildings(int modelType)
	if (gu->spectatingFullView)
		return;

	const auto& deadGhostBuildings = unitDrawerData->GetDeadGhostBuildings(gu->myAllyTeam, modelType);

	// deadGhostedBuildings
	{
		SetColorMultiplier(0.6f, 0.6f, 0.6f, alphaValues.y);
		SetDrawingMode(ShaderDrawingModes::STATIC_MODEL);

		int prevModelType = -1;
		int prevTexType = -1;
		for (const auto* dgb : deadGhostBuildings) {
			if (!camera->InView(dgb->pos, dgb->model->GetDrawRadius()))
				continue;

			static CMatrix44f staticWorldMat;

			staticWorldMat.LoadIdentity();
			staticWorldMat.Translate(dgb->pos);

			staticWorldMat.RotateY(math::DEG_TO_RAD * 90.0f);

			if (prevModelType != modelType || prevTexType != dgb->model->textureType) {
				prevModelType = modelType; prevTexType = dgb->model->textureType;
				BindModelTypeTexture(modelType, dgb->model->textureType); //ineficient rendering, but w/e
			}

			SetStaticModelMatrix(staticWorldMat);
			smv.SubmitImmediately(dgb->model, dgb->team); //need to submit immediately every model because of static per-model matrix
		}
	}

	// liveGhostedBuildings
	{
		const auto& liveGhostedBuildings = unitDrawerData->GetLiveGhostBuildings(gu->myAllyTeam, modelType);

		int prevModelType = -1;
		int prevTexType = -1;
		for (const auto* lgb : liveGhostedBuildings) {
			if (!camera->InView(lgb->pos, lgb->model->GetDrawRadius()))
				continue;

			// check for decoy models
			const UnitDef* decoyDef = lgb->unitDef->decoyDef;
			const S3DModel* model = nullptr;

			if (decoyDef == nullptr) {
				model = lgb->model;
			}
			else {
				model = decoyDef->LoadModel();
			}

			// FIXME: needs a second pass
			if (model->type != modelType)
				continue;

			static CMatrix44f staticWorldMat;

			staticWorldMat.LoadIdentity();
			staticWorldMat.Translate(lgb->pos);

			staticWorldMat.RotateY(math::DEG_TO_RAD * 90.0f);

			const unsigned short losStatus = lgb->losStatus[gu->myAllyTeam];

			// ghosted enemy units
			if (losStatus & LOS_CONTRADAR)
				SetColorMultiplier(0.9f, 0.9f, 0.9f, alphaValues.z);
			else
				SetColorMultiplier(0.6f, 0.6f, 0.6f, alphaValues.y);

			if (prevModelType != modelType || prevTexType != model->textureType) {
				prevModelType = modelType; prevTexType = model->textureType;
				BindModelTypeTexture(modelType, model->textureType); //ineficient rendering, but w/e
			}

			SetStaticModelMatrix(staticWorldMat);
			smv.SubmitImmediately(model, lgb->team); //need to submit immediately every model because of static per-model matrix
		}
	}

	smv.Unbind();
}

void CUnitDrawerGL4::Enable(bool deferredPass, bool alphaPass) const
{
	// body of former EnableCommon();
	CUnitDrawerHelper::EnableTexturesCommon();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);
	assert(modelShader != nullptr);
	modelShader->Enable();

	switch (game->GetDrawMode())
	{
		case CGame::GameDrawMode::gameReflectionDraw: {
			glEnable(GL_CLIP_DISTANCE2);
			SetDrawingMode(ShaderDrawingModes::REFLCT_MODEL);
		} break;
		case CGame::GameDrawMode::gameRefractionDraw: {
			glEnable(GL_CLIP_DISTANCE2);
			SetDrawingMode(ShaderDrawingModes::REFRAC_MODEL);
		} break;
		default: SetDrawingMode(ShaderDrawingModes::NORMAL_MODEL); break;
	}

	float gtThreshold = mix(0.5, 0.1, static_cast<float>(alphaPass));
	modelShader->SetUniform("alphaCtrl", gtThreshold, 1.0f, 0.0f, 0.0f); // test > 0.1 | 0.5

	// end of EnableCommon();
}

void CUnitDrawerGL4::Disable(bool deferredPass) const
{
	assert(modelShader != nullptr);

	modelShader->Disable();

	SetActiveShader(shadowHandler.ShadowsLoaded(), deferredPass);

	switch (game->GetDrawMode())
	{
	case CGame::GameDrawMode::gameReflectionDraw: {
		glDisable(GL_CLIP_DISTANCE2);
	} break;
	case CGame::GameDrawMode::gameRefractionDraw: {
		glDisable(GL_CLIP_DISTANCE2);
	} break;
	default: {} break;
	}

	CUnitDrawerHelper::DisableTexturesCommon();
}

void CUnitDrawerGL4::EnableTextures() const { CUnitDrawerHelper::EnableTexturesCommon(); }
void CUnitDrawerGL4::DisableTextures() const { CUnitDrawerHelper::DisableTexturesCommon(); }


void CUnitDrawerGL4::SetColorMultiplier(float r, float g, float b, float a) const
{
	assert(modelShader->IsBound());
	modelShader->SetUniform("colorMult", r, g, b, a);
}

void CUnitDrawerGL4::SetDrawingMode(ShaderDrawingModes sdm) const
{
	assert(modelShader->IsBound());
	modelShader->SetUniform("drawMode", static_cast<int>(sdm));

	switch (sdm)
	{
		case CUnitDrawerGL4::REFLCT_MODEL:
			modelShader->SetUniform("waterClipPlane", 0.0f,  1.0f, 0.0f, 0.0f);
			break;
		case CUnitDrawerGL4::REFRAC_MODEL:
			modelShader->SetUniform("waterClipPlane", 0.0f, -1.0f, 0.0f, 0.0f);
			break;
		default:
			modelShader->SetUniform("waterClipPlane", 0.0f, 0.0f, 0.0f, 1.0f);
			break;
	}
}

void CUnitDrawerGL4::SetStaticModelMatrix(const CMatrix44f& mat) const
{
	assert(modelShader->IsBound());
	modelShader->SetUniformMatrix4x4("staticModelMatrix", false, &mat.m[0]);
}

bool CUnitDrawerGL4::CheckLegacyDrawing(const CUnit* unit, bool noLuaCall) const
{
	if (unit->luaDraw || !noLuaCall)
		return false;

	return true;
}

bool CUnitDrawerGL4::CheckLegacyDrawing(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const
{
	if (forceLegacyPath)
		return false;

	if (lodCall || preList != 0 || postList != 0 || CheckLegacyDrawing(unit, noLuaCall)) { //TODO: sanitize
		ForceLegacyPath();
		return false;
	}

	return true;
}

void CUnitDrawerGL4::SetupOpaqueDrawing(bool deferredPass) const
{
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireFrameMode + GL_FILL * (1 - wireFrameMode));

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	//alpha test > 0.5

	Enable(deferredPass, false);
}

void CUnitDrawerGL4::ResetOpaqueDrawing(bool deferredPass) const
{
	Disable(deferredPass);
	glPopAttrib();
}

void CUnitDrawerGL4::SetupAlphaDrawing(bool deferredPass) const
{
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE * wireFrameMode + GL_FILL * (1 - wireFrameMode));

	Enable(/*deferredPass always false*/ false, true);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//alpha test > 0.1

	glDepthMask(GL_FALSE);
}

void CUnitDrawerGL4::ResetAlphaDrawing(bool deferredPass) const
{
	Disable(/*deferredPass*/ false);
	glPopAttrib();
}

bool CUnitDrawerGL4::SetTeamColour(int team, const float2 alpha) const
{
	if (!CUnitDrawer::SetTeamColour(team, alpha))
		return false;

	//todo

	return true;
}

