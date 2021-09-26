/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <array>

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

enum UnitDrawerTypes {
	UNIT_DRAWER_FFP  = 0, // fixed-function path
	UNIT_DRAWER_ARB  = 1, // standard-shader path (ARB)
	UNIT_DRAWER_GLSL = 2, // standard-shader path (GLSL)
	UNIT_DRAWER_GL4  = 3, // GL4-shader path (GLSL)
	UNIT_DRAWER_CNT  = 4
};

static const std::string UnitDrawerNames[UnitDrawerTypes::UNIT_DRAWER_CNT] = {
	"FFP : fixed-function path",
	"ARB : legacy standard shader path",
	"GLSL: legacy standard shader path",
	"GL4 : modern standard shader path",
};

class CUnitDrawer
{
public:
	CUnitDrawer() {}
	virtual ~CUnitDrawer() {}
public:
	template<typename T>
	static void InitInstance(int t) {
		if (unitDrawers[t] == nullptr)
			unitDrawers[t] = new T{};
	}
	static void KillInstance(int t) {
		spring::SafeDelete(unitDrawers[t]);
	}

	static void InitStatic();
	static void KillStatic(bool reload);

	static void ForceLegacyPath();

	static void SelectImplementation(bool forceReselection = false);
	static void SelectImplementation(int targetImplementation);

	static void UpdateStatic();

	// Set/Get state from outside
	static void SetDrawForwardPass (bool b) { drawForward = b; }
	static void SetDrawDeferredPass(bool b) { drawDeferred = b; }
	static bool DrawForward () { return drawForward; }
	static bool DrawDeferred() { return drawDeferred; }

	static bool  UseAdvShading   () { return advShading; }
	static bool& UseAdvShadingRef() { reselectionRequested = true; return advShading; }
	static bool& WireFrameModeRef() { return wireFrameMode; }

	static int  PreferedDrawerType   () { return preferedDrawerType; }
	static int& PreferedDrawerTypeRef() { reselectionRequested = true; return preferedDrawerType; }

	static bool& MTDrawerTypeRef() { return mtModelDrawer; } //no reselectionRequested needed
public:
	// Interface with CUnitDrawerData
	static void SunChangedStatic();

	static void UpdateGhostedBuildings() { unitDrawerData->UpdateGhostedBuildings(); }

	static uint32_t GetUnitDefImage(const UnitDef* ud) { return unitDrawerData->GetUnitDefImage(ud); }
	static void SetUnitDefImage(const UnitDef* unitDef, const std::string& texName) { return unitDrawerData->SetUnitDefImage(unitDef, texName); }
	static void SetUnitDefImage(const UnitDef* unitDef, uint32_t texID, int xsize, int ysize) { return unitDrawerData->SetUnitDefImage(unitDef, texID, xsize, ysize); }

	static bool& UseScreenIcons() { return unitDrawerData->useScreenIcons; }

	static float GetUnitIconFadeStart() { return unitDrawerData->GetUnitIconFadeStart(); }
	static void SetUnitIconFadeStart(float scale) { unitDrawerData->SetUnitIconFadeStart(scale); }

	static float GetUnitIconScaleUI() { return unitDrawerData->GetUnitIconScaleUI(); }
	static void SetUnitIconScaleUI(float scale) { unitDrawerData->SetUnitIconScaleUI(scale); }

	static float GetUnitDrawDist() { return unitDrawerData->unitDrawDist; }
	static void SetUnitDrawDist(float dist) { unitDrawerData->SetUnitDrawDist(dist); }

	static float GetUnitIconDist(float dist) { return unitDrawerData->unitIconDist; }
	static void SetUnitIconDist(float dist) { unitDrawerData->SetUnitIconDist(dist); }

	static float GetUnitIconFadeVanish() { return unitDrawerData->iconFadeVanish; }
	static void SetUnitIconFadeVanish(float dist) { unitDrawerData->SetUnitIconFadeVanish(dist); }

	static bool& IconHideWithUI() { return unitDrawerData->iconHideWithUI; }

	static void AddTempDrawUnit(const CUnitDrawerData::TempDrawUnit& tempDrawUnit) { unitDrawerData->AddTempDrawUnit(tempDrawUnit); };

	static const std::vector<CUnit*>& GetUnsortedUnits() { return unitDrawerData->GetUnsortedUnits(); }
	static const ScopedMatricesMemAlloc& GetUnitMatricesMemAlloc(const CUnit* unit) { return unitDrawerData->GetObjectMatricesMemAlloc(unit); }
public:
	virtual void SunChanged() const = 0;
	virtual void Update() const = 0;

	// Former UnitDrawerState + new functions
	virtual bool CanEnable() const = 0;
	virtual bool CanDrawDeferred() const = 0;
	virtual bool CanDrawAlpha() const = 0; //only used by feature drawer (legacy)

	virtual bool IsLegacy() const = 0;

	// Setup Fixed State
	virtual void SetupOpaqueDrawing(bool deferredPass) const = 0;
	virtual void ResetOpaqueDrawing(bool deferredPass) const = 0;

	virtual void SetupAlphaDrawing(bool deferredPass) const = 0;
	virtual void ResetAlphaDrawing(bool deferredPass) const = 0;

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

	// Draw*
	virtual void Draw(bool drawReflection, bool drawRefraction = false) const = 0;
	virtual void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const = 0;
	virtual void DrawShadowPass() const = 0;
	virtual void DrawAlphaPass() const = 0;

	// Icons Minimap
	virtual void DrawUnitMiniMapIcons() const = 0;
	        void UpdateUnitDefMiniMapIcons(const UnitDef* ud) { unitDrawerData->UpdateUnitDefMiniMapIcons(ud); }

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
	// lightHandler
	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	// geomBuffer
	const GL::GeometryBuffer* GetGeometryBuffer() const { return geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return geomBuffer; }
public:
	// Render States Push/Pop
	static void BindModelTypeTexture(int mdlType, int texType);

	static void PushModelRenderState(int mdlType);
	static void PushModelRenderState(const S3DModel* m);
	static void PushModelRenderState(const CSolidObject* o);

	static void PopModelRenderState(int mdlType);
	static void PopModelRenderState(const S3DModel* m);
	static void PopModelRenderState(const CSolidObject* o);

	// Auxilary
	static bool ObjectVisibleReflection(const float3 objPos, const float3 camPos, float maxRadius);
public:
	/// <summary>
	/// .x := regular unit alpha
	/// .y := ghosted unit alpha (out of radar)
	/// .z := ghosted unit alpha (inside radar)
	/// .w := AI-temp unit alpha
	/// </summary>
	inline static float4 alphaValues = {}; //TODO move me to protected when UnitDrawerState is gone
protected:
	inline static int preferedDrawerType = UnitDrawerTypes::UNIT_DRAWER_CNT;
	inline static bool mtModelDrawer = true;

	inline static bool forceLegacyPath = false;

	inline static bool wireFrameMode = false;

	inline static bool drawForward = true;
	inline static bool drawDeferred = true;

	inline static bool deferredAllowed = false;

	inline static CUnitDrawerData* unitDrawerData;
private:
	inline static bool advShading = false;

	inline static std::array<CUnitDrawer*, UnitDrawerTypes::UNIT_DRAWER_CNT> unitDrawers = {};
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
private:
	inline static bool reselectionRequested = true;
	//inline static int selectedImplementation = UnitDrawerTypes::UNIT_DRAWER_FFP;
	inline static GL::LightHandler lightHandler;
	inline static GL::GeometryBuffer* geomBuffer = nullptr;
};

class CUnitDrawerBase : public CUnitDrawer {
public:
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override;
	void DrawAlphaPass() const override;
	void Update() const override;
protected:
	template<bool legacy>
	void DrawShadowPassImpl() const;

	template<bool legacy>
	void DrawImpl(bool drawReflection, bool drawRefraction) const;
};

class CUnitDrawerLegacy : public CUnitDrawerBase {
public:
	void SunChanged() const override {}
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
	void SunChanged() const override {}

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

extern CUnitDrawer* unitDrawer;