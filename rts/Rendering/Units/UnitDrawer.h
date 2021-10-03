/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <array>

#include "Rendering/Common/ModelDrawer.h"
#include "Rendering/Units/UnitDrawerData.h"
#include "Rendering/GL/LightHandler.h"
#include "System/type2.h"
#include "Sim/Units/CommandAI/Command.h"

class CSolidObject;
class CUnit;
struct S3DModel;
struct SolidObjectDef;
class ScopedMatricesMemAlloc;

namespace Shader { struct IProgramObject; }

class CUnitDrawer : public CModelDrawerBase<CUnitDrawerData, CUnitDrawer>
{
public:
	friend class CModelDrawerBase<CUnitDrawerData, CUnitDrawer>;
public:
	CUnitDrawer() {}
	virtual ~CUnitDrawer() {}
public:
	static void InitStatic();
	//static void KillStatic(bool reload); //use base
	//static void UpdateStatic(); //use base
public:
	// Interface with CUnitDrawerData
	static void UpdateGhostedBuildings() { modelDrawerData->UpdateGhostedBuildings(); }

	static uint32_t GetUnitDefImage(const UnitDef* ud) { return modelDrawerData->GetUnitDefImage(ud); }
	static void SetUnitDefImage(const UnitDef* unitDef, const std::string& texName) { return modelDrawerData->SetUnitDefImage(unitDef, texName); }
	static void SetUnitDefImage(const UnitDef* unitDef, uint32_t texID, int xsize, int ysize) { return modelDrawerData->SetUnitDefImage(unitDef, texID, xsize, ysize); }

	static bool& UseScreenIcons() { return modelDrawerData->useScreenIcons; }

	static float GetUnitIconFadeStart() { return modelDrawerData->GetUnitIconFadeStart(); }
	static void SetUnitIconFadeStart(float scale) { modelDrawerData->SetUnitIconFadeStart(scale); }

	static float GetUnitIconScaleUI() { return modelDrawerData->GetUnitIconScaleUI(); }
	static void SetUnitIconScaleUI(float scale) { modelDrawerData->SetUnitIconScaleUI(scale); }

	static float GetUnitDrawDist() { return CModelRenderDataConcept::modelDrawDist; }
	static void SetModelDrawDist(float dist) { CModelRenderDataConcept::SetModelDrawDist(dist); }

	static float GetUnitIconDist(float dist) { return modelDrawerData->unitIconDist; }
	static void SetUnitIconDist(float dist) { modelDrawerData->SetUnitIconDist(dist); }

	static float GetUnitIconFadeVanish() { return modelDrawerData->iconFadeVanish; }
	static void SetUnitIconFadeVanish(float dist) { modelDrawerData->SetUnitIconFadeVanish(dist); }

	static bool& IconHideWithUI() { return modelDrawerData->iconHideWithUI; }

	static void AddTempDrawUnit(const CUnitDrawerData::TempDrawUnit& tempDrawUnit) { modelDrawerData->AddTempDrawUnit(tempDrawUnit); };

	static const std::vector<CUnit*>& GetUnsortedUnits() { return modelDrawerData->GetUnsortedObjects(); }
	static const ScopedMatricesMemAlloc& GetUnitMatricesMemAlloc(const CUnit* unit) { return modelDrawerData->GetObjectMatricesMemAlloc(unit); }
protected:
	virtual void Update() const = 0;
public:
	//virtual bool CanEnable() const = 0; //moved to CModelDrawerBase

	// Former UnitDrawerState + new functions
	virtual bool CanDrawDeferred() const = 0;
	virtual bool CanDrawAlpha() const = 0; //only used by feature drawer (legacy), TODO: remove

	virtual bool IsLegacy() const = 0; //TODO unused apparently

	// alpha.x := alpha-value
	// alpha.y := alpha-pass (true or false)
	virtual bool SetTeamColour(int team, const float2 alpha = float2(1.0f, 0.0f)) const;

	// DrawUnit*
	virtual void DrawUnitModel(const CUnit* unit, bool noLuaCall) const = 0;
	virtual void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const = 0;
	virtual void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const = 0;
	virtual void DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	virtual void DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	virtual void DrawIndividual(const CUnit* unit, bool noLuaCall) const = 0;
	virtual void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const = 0;

	// DrawIndividualDef*
	virtual void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;
	virtual void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;

	// Draw
	virtual void Draw(bool drawReflection, bool drawRefraction = false) const = 0;

	// Icons Minimap
	virtual void DrawUnitMiniMapIcons() const = 0;
	        void UpdateUnitDefMiniMapIcons(const UnitDef* ud) { modelDrawerData->UpdateUnitDefMiniMapIcons(ud); }

	// Icons Map
	virtual void DrawUnitIcons() const = 0;
	virtual void DrawUnitIconsScreen() const = 0;

	// Build Squares
	        bool ShowUnitBuildSquare(const BuildInfo& buildInfo) const { return ShowUnitBuildSquare(buildInfo, std::vector<Command>()); }
	virtual bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands) const = 0;
protected:
	bool CanDrawOpaqueUnit(const CUnit* unit, bool drawReflection, bool drawRefraction) const;
	bool ShouldDrawOpaqueUnit(const CUnit* unit, bool drawReflection, bool drawRefraction) const;
	bool ShouldDrawAlphaUnit(CUnit* unit) const;
	bool CanDrawOpaqueUnitShadow(const CUnit* unit) const;
	bool ShouldDrawOpaqueUnitShadow(CUnit* unit) const;

	virtual void DrawOpaqueUnitsShadow(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const = 0;
	virtual void DrawOpaqueUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const = 0;

	virtual void DrawAlphaUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const = 0;

	virtual void DrawOpaqueAIUnits(int modelType) const = 0;
	virtual void DrawAlphaAIUnits(int modelType) const = 0;

	virtual void DrawGhostedBuildings(int modelType) const = 0;
protected:
	virtual void Enable(bool deferredPass, bool alphaPass) const = 0;
	virtual void Disable(bool deferredPass) const = 0;

	virtual void SetNanoColor(const float4& color) const = 0;
public:
	/// <summary>
	/// .x := regular unit alpha
	/// .y := ghosted unit alpha (out of radar)
	/// .z := ghosted unit alpha (inside radar)
	/// .w := AI-temp unit alpha
	/// </summary>
	inline static float4 alphaValues = {}; //TODO move me to protected when UnitDrawerState is gone

private:
	inline static std::array<CUnitDrawer*, ModelDrawerTypes::MODEL_DRAWER_CNT> unitDrawers = {};
public:
	enum BuildStages {
		BUILDSTAGE_WIRE = 0,
		BUILDSTAGE_FLAT = 1,
		BUILDSTAGE_FILL = 2,
		BUILDSTAGE_NONE = 3,
		BUILDSTAGE_CNT = 4,
	};
	enum ModelShaderProgram {
		MODEL_SHADER_NOSHADOW_STANDARD = 0, ///< model shader (V+F) without self-shadowing
		MODEL_SHADER_SHADOWED_STANDARD = 1, ///< model shader (V+F) with    self-shadowing
		MODEL_SHADER_NOSHADOW_DEFERRED = 2, ///< deferred version of MODEL_SHADER_NOSHADOW (GLSL-only)
		MODEL_SHADER_SHADOWED_DEFERRED = 3, ///< deferred version of MODEL_SHADER_SHADOW   (GLSL-only)
		MODEL_SHADER_COUNT = 4,
	};
};

class CUnitDrawerBase : public CUnitDrawer {
public:
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override;
	void DrawAlphaPass() const override;
protected:
	void Update() const override;
protected:
	template<bool legacy>
	void DrawShadowPassImpl() const;

	template<bool legacy>
	void DrawImpl(bool drawReflection, bool drawRefraction) const;
};

class CUnitDrawerLegacy : public CUnitDrawerBase {
public:
	// caps functions
	bool IsLegacy() const override { return true; }
	// Inherited via CUnitDrawer
	void SetupOpaqueDrawing(bool deferredPass) const override;
	void ResetOpaqueDrawing(bool deferredPass) const override;

	void SetupAlphaDrawing(bool deferredPass) const override;
	void ResetAlphaDrawing(bool deferredPass) const override;

	void DrawUnitModel(const CUnit* unit, bool noLuaCall) const override;
	void DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const override;
	void DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const override;
	void DrawIndividual(const CUnit* unit, bool noLuaCall) const override;
	void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const override;

	void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const override;
	void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const override;

	bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands) const override;


	void Draw(bool drawReflection, bool drawRefraction = false) const override { DrawImpl<true>(drawReflection, drawRefraction); }
	void DrawShadowPass() const override { DrawShadowPassImpl<true>(); }

	void DrawUnitMiniMapIcons() const override;
	void DrawUnitIcons() const override;
	void DrawUnitIconsScreen() const override;
protected:
	void DrawOpaqueUnitsShadow(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override;
	void DrawOpaqueUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const override;

	void DrawAlphaUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override;

	void DrawOpaqueAIUnits(int modelType) const override;
	void DrawAlphaAIUnits(int modelType) const override;

	void DrawGhostedBuildings(int modelType) const override;

	virtual void EnableTextures() const = 0;
	virtual void DisableTextures() const = 0;

	void DrawOpaqueUnit(CUnit* unit, bool drawReflection, bool drawRefraction) const;
	void DrawOpaqueUnitShadow(CUnit* unit) const;
	void DrawAlphaUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass) const;
	void DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;
	void DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;
	void DrawAlphaAIUnitBorder(const CUnitDrawerData::TempDrawUnit& unit) const;

	void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const override;
	void DrawModelWireBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall, bool amdHack) const;
	void DrawModelFlatBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;
	void DrawModelFillBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;

	void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const override;
	void DrawModelWireBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall, bool amdHack) const;
	void DrawModelFlatBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;
	void DrawModelFillBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall, bool amdHack) const;

	void PushIndividualOpaqueState(const CUnit* unit, bool deferredPass) const;
	void PushIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass)  const;
	void PushIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const;

	void PopIndividualOpaqueState(const CUnit* unit, bool deferredPass) const;
	void PopIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass) const;
	void PopIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const;

	void DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const;

	static void DrawIcon(CUnit* unit, bool useDefaultIcon);
	void DrawIconScreenArray(const CUnit* unit, const icon::CIconData* icon, bool useDefaultIcon, const float dist, CVertexArray* va) const;

	void SetActiveShader(bool shadowed, bool deferred) const {
		// shadowed=1 --> shader 1 (deferred=0) or 3 (deferred=1)
		// shadowed=0 --> shader 0 (deferred=0) or 2 (deferred=1)
		modelShader = modelShaders[shadowed + deferred * 2];
	}
protected:
	std::array<Shader::IProgramObject*, MODEL_SHADER_COUNT> modelShaders = {};
	mutable Shader::IProgramObject* modelShader = nullptr;
};

class CUnitDrawerFFP final : public CUnitDrawerLegacy {
public:
	CUnitDrawerFFP() {}
	~CUnitDrawerFFP() override {}
public:
	// caps functions
	bool CanEnable() const override { return true; }
	bool CanDrawDeferred() const override { return false; }
	bool CanDrawAlpha() const override { return false; } //by legacy convention FFP is not alpha capable

	bool SetTeamColour(int team, const float2 alpha = float2(1.0f, 0.0f)) const override;

protected:
	void Enable(bool deferredPass, bool alphaPass) const override;
	void Disable(bool deferredPass) const override;
	void SetNanoColor(const float4& color) const override;

	void EnableTextures() const override;
	void DisableTextures() const override;
private:
public:
	// TODO move back to private when DrawerState is gone
	// needed by FFP drawer-state
	static void SetupBasicS3OTexture0();
	static void SetupBasicS3OTexture1();
	static void CleanupBasicS3OTexture1();
	static void CleanupBasicS3OTexture0();
};

class CUnitDrawerARB final : public CUnitDrawerLegacy {
public:
	CUnitDrawerARB();
	~CUnitDrawerARB() override;
public:
	// caps functions
	bool CanEnable() const override;
	bool CanDrawDeferred() const override { return false; };
	bool CanDrawAlpha() const override { return false; } //by legacy convention ARB is not alpha capable?

	bool SetTeamColour(int team, const float2 alpha = float2(1.0f, 0.0f)) const override;

protected:
	void Enable(bool deferredPass, bool alphaPass) const override;
	void Disable(bool deferredPass) const override;
	void SetNanoColor(const float4& color) const override;

	void EnableTextures() const override;
	void DisableTextures() const override;
};

class CUnitDrawerGLSL final : public CUnitDrawerLegacy {
public:
	CUnitDrawerGLSL();
	~CUnitDrawerGLSL() override;
public:
	// caps functions
	bool CanEnable() const override;
	bool CanDrawDeferred() const override;
	bool CanDrawAlpha() const override { return false; } //by legacy convention ARB is not alpha capable?

	bool SetTeamColour(int team, const float2 alpha = float2(1.0f, 0.0f)) const override;

protected:
	void Enable(bool deferredPass, bool alphaPass) const override;
	void Disable(bool deferredPass) const override;
	void SetNanoColor(const float4& color) const override;

	void EnableTextures() const override;
	void DisableTextures() const override;
};

class CUnitDrawerGL4 final : public CUnitDrawerLegacy {
public:
	CUnitDrawerGL4();
	~CUnitDrawerGL4() override;
public:
	// Former UnitDrawerState + new functions
	bool CanEnable() const;
	bool CanDrawDeferred() const;
	bool CanDrawAlpha() const { return true; }

	bool IsLegacy() const override { return false; }

	// Setup Fixed State
	void SetupOpaqueDrawing(bool deferredPass) const override;
	void ResetOpaqueDrawing(bool deferredPass) const override;

	void SetupAlphaDrawing(bool deferredPass) const override;
	void ResetAlphaDrawing(bool deferredPass) const override;

	bool SetTeamColour(int team, const float2 alpha = float2(1.0f, 0.0f)) const override;

	// DrawUnit*
	/* TODO figure out
	void DrawUnitModel(const CUnit* unit, bool noLuaCall) const = 0;
	void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const = 0;
	void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const = 0;
	void DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	void DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	void DrawIndividual(const CUnit* unit, bool noLuaCall) const = 0;
	void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const = 0;
	*/

	// DrawIndividualDef*
	/*
	void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;
	void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;
	*/

	void Draw(bool drawReflection, bool drawRefraction = false) const override { DrawImpl<false>(drawReflection, drawRefraction); }
	void DrawShadowPass() const override { DrawShadowPassImpl<false>(); }

protected:
	void DrawOpaqueUnitsShadow(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override;
	void DrawOpaqueUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType, bool drawReflection, bool drawRefraction) const override;

	void DrawAlphaUnits(const CUnitRenderDataBase::RdrContProxy& rdrCntProxy, int modelType) const override;

	void DrawOpaqueAIUnits(int modelType) const override {};
	void DrawAlphaAIUnits(int modelType) const override {};

	void DrawGhostedBuildings(int modelType) const override {};

	void Enable(bool deferredPass, bool alphaPass) const override;
	void Disable(bool deferredPass) const override;
	void SetNanoColor(const float4& color) const override {};

	void EnableTextures() const override;
	void DisableTextures() const override;
private:
	enum ShaderDrawingModes {
		STATIC_MODEL = -1,
		NORMAL_MODEL =  0,
		REFLCT_MODEL =  1,
		REFRAC_MODEL =  2,
	};
private:
	void SetColorMultiplier(float a = 1.0f) const { SetColorMultiplier(1.0f, 1.0f, 1.0f, a); };
	void SetColorMultiplier(float r, float g, float b, float a) const;

	void SetDrawingMode(ShaderDrawingModes sdm) const;
	void SetStaticModelMatrix(const CMatrix44f& mat) const;

	bool CheckLegacyDrawing(const CUnit* unit, bool noLuaCall) const;
	bool CheckLegacyDrawing(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const;

};

#define unitDrawer (CUnitDrawer::selectedModelDrawer)