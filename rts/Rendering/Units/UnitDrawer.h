/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <vector>
#include <array>

#include "Rendering/Common/ModelDrawer.h"
#include "Rendering/Common/ModelDrawerState.hpp"
#include "Rendering/Units/UnitDrawerData.h"
#include "Rendering/GL/LightHandler.h"
#include "Game/UI/CursorIcons.h"
#include "System/type2.h"
#include "Sim/Units/CommandAI/Command.h"

class CSolidObject;
class CUnit;
struct S3DModel;
struct SolidObjectDef;

namespace Shader { struct IProgramObject; }

// viewer and style state for one unit-icon minimap pass; built by CMiniMap from its
// own settings and by gl.DrawMiniMapIcons from its arguments. Icons are emitted as
// quads in map-space elmo coordinates (XZ), transformed by the current GL matrices.
struct MiniMapIconDrawParams {
	float iconSizeX = 1.0f;                  // base icon half-size in elmos, scaled per icon
	float iconSizeY = 1.0f;
	int rotation = 0;                        // CMiniMap::RotationOptions
	int viewAllyTeam = 0;                    // perspective the icons are selected/positioned for
	bool fullView = false;                   // godmode: true positions, everything visible
	bool useIcons = true;                    // false = draw every unit as the default radar dot
	bool useSimpleColors = false;            // use the three fixed colors instead of team colors
	// draw the LOCAL player's selected units white; other players' selections are not
	// known engine-side, so callers rendering another perspective disable this and
	// overlay that viewer's selection themselves
	bool highlightSelected = true;
	SColor myColor = SColor(0, 255, 0, 255);
	SColor allyColor = SColor(0, 255, 255, 255);
	SColor enemyColor = SColor(255, 0, 0, 255);
	// map-space cull rect (elmos); defaults draw everything
	float cullMinX = -1e9f, cullMinZ = -1e9f;
	float cullMaxX = 1e9f, cullMaxZ = 1e9f;
};

class CUnitDrawer : public CModelDrawerBase<CUnitDrawerData, CUnitDrawer>
{
public:
	static void InitStatic();
	static void KillStatic(bool reload); //use base
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

	static float GetUnitIconDist(float dist) { return modelDrawerData->unitIconDist; }
	static void SetUnitIconDist(float dist) { modelDrawerData->SetUnitIconDist(dist); }

	static float GetUnitIconFadeVanish() { return modelDrawerData->iconFadeVanish; }
	static void SetUnitIconFadeVanish(float dist) { modelDrawerData->SetUnitIconFadeVanish(dist); }

	static bool& IconHideWithUI() { return modelDrawerData->iconHideWithUI; }

	static void AddTempDrawUnit(const CUnitDrawerData::TempDrawUnit& tempDrawUnit) { modelDrawerData->AddTempDrawUnit(tempDrawUnit); }

	static const std::vector<CUnit*>& GetUnsortedUnits() { return modelDrawerData->GetUnsortedObjects(); }

	static void ClearPreviousDrawFlags() { modelDrawerData->ClearPreviousDrawFlags(); }
	static void UnitLeavesGhostChanged(const CUnit* unit, const bool leaveDeadGhost) { modelDrawerData->UnitLeavesGhostChanged(unit, leaveDeadGhost); }

	static void UpdateCurrentUnitIcon(const CUnit* unit) { modelDrawerData->UpdateCurrentUnitIcon(unit); }
public:
	// DrawUnit*
	virtual void DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	virtual void DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const = 0;
	virtual void DrawIndividual(const CUnit* unit, bool noLuaCall) const = 0;
	virtual void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const = 0;

	// DrawIndividualDef*
	virtual void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;
	virtual void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const = 0;

	// Icons Minimap
	virtual void DrawUnitMiniMapIcons(const MiniMapIconDrawParams& params) const = 0;
	        void UpdateUnitIconsByUnitDef(const UnitDef* ud) { modelDrawerData->UpdateUnitIconsByUnitDef(ud); }

	// Icons Map
	virtual void DrawUnitIcons() const = 0;
	virtual void DrawUnitIconsScreen() const = 0;

	// Build Squares
	        bool ShowUnitBuildSquare(const BuildInfo& buildInfo) const { return ShowUnitBuildSquare(buildInfo, std::vector<Command>()); }
	virtual bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands) const = 0;

	virtual void DrawBuildIcons(const std::vector<CCursorIcons::BuildIcon>& buildIcons) const = 0;
protected:
	static bool ShouldDrawOpaqueUnit(CUnit* u, uint8_t thisPassMask);
	static bool ShouldDrawAlphaUnit(CUnit* u, uint8_t thisPassMask);
	static bool ShouldDrawUnitShadow(CUnit* u);

	virtual void DrawGhostedBuildings(int modelType) const = 0;
protected:
	inline static Shader::IProgramObject* icons2DShader = nullptr;
	inline static Shader::IProgramObject* icons3DShader = nullptr;
private:
	inline static std::array<CUnitDrawer*, ModelDrawerTypes::MODEL_DRAWER_CNT> unitDrawers = {};

	inline static bool engineBuildSquareRendering = true;
public:
	static bool& EngineBuildSquareRendering() { return engineBuildSquareRendering; }

	enum BuildStages {
		BUILDSTAGE_WIRE = 0,
		BUILDSTAGE_FLAT = 1,
		BUILDSTAGE_FILL = 2,
		BUILDSTAGE_NONE = 3,
		BUILDSTAGE_CNT = 4,
	};
};

class CUnitDrawerBase : public CUnitDrawer {
public:
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction) const override {
		DrawOpaquePassImpl<LuaObjType::LUAOBJ_UNIT>(deferredPass, drawReflection, drawRefraction);
	}
	void DrawAlphaPass(bool drawReflection, bool drawRefraction = false) const override {
		DrawAlphaPassImpl<LuaObjType::LUAOBJ_UNIT>(drawReflection, drawRefraction);
	}
protected:
	void DrawOpaqueObjectsLua(bool deferredPass, bool drawReflection, bool drawRefraction) const override {
		eventHandler.DrawOpaqueUnitsLua(deferredPass, drawReflection, drawRefraction);
	}
	void DrawAlphaObjectsLua(bool drawReflection, bool drawRefraction) const override {
		eventHandler.DrawAlphaUnitsLua(drawReflection, drawRefraction);
	}
	void DrawShadowObjectsLua() const override {
		eventHandler.DrawShadowUnitsLua();
	}
	void Update() const override;
};

class CUnitDrawerGLSL : public CUnitDrawerBase {
public:
	CUnitDrawerGLSL();
	~CUnitDrawerGLSL() override;
public:
	void Draw(bool drawReflection, bool drawRefraction = false) const override {
		DrawImpl<true, LuaObjType::LUAOBJ_UNIT>(drawReflection, drawRefraction);
	}
	void DrawShadowPass() const override {
		DrawShadowPassImpl<true, LuaObjType::LUAOBJ_UNIT>();
	}

	void DrawUnitModel(const CUnit* unit, bool noLuaCall) const;

	void DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const override;
	void DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const override;
	void DrawIndividual(const CUnit* unit, bool noLuaCall) const override;
	void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const override;

	void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const override;
	void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false) const override;

	bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands) const override;
	void DrawBuildIcons(const std::vector<CCursorIcons::BuildIcon>& buildIcons) const override;

	void DrawUnitMiniMapIcons(const MiniMapIconDrawParams& params) const override;
	void DrawUnitIcons() const override;
	void DrawUnitIconsScreen() const override;
protected:
	void DrawObjectsShadow(int modelType) const override;
	void DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const override;
	void DrawOpaqueObjectsAux(int modelType) const override; //AI units

	void DrawAlphaObjects(int modelType, bool drawReflection, bool drawRefraction) const override;
	void DrawAlphaObjectsAux(int modelType) const override;

	void DrawGhostedBuildings(int modelType) const override;

	void DrawOpaqueUnit(CUnit* unit, uint8_t thisPassMask) const;
	void DrawUnitShadow(CUnit* unit) const;
	void DrawAlphaUnit(CUnit* unit, uint8_t thisPassMask) const;

	void DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;
	void DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;
	void DrawAlphaAIUnitBorder(const CUnitDrawerData::TempDrawUnit& unit) const;

	void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const;
	void DrawModelWireBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;
	void DrawModelFlatBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;
	void DrawModelFillBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;

	void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const;
	void DrawModelWireBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;
	void DrawModelFlatBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;
	void DrawModelFillBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const;

	void PushIndividualOpaqueState(const CUnit* unit, bool deferredPass) const;
	void PushIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass)  const;
	void PushIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const;

	void PopIndividualOpaqueState(const CUnit* unit, bool deferredPass) const;
	void PopIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass) const;
	void PopIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const;

	void DrawUnitMiniMapIcon(TypedRenderBuffer<VA_TYPE_2DTC3>& rb, size_t iconIdx, const float iconScale, const float3& pos, const SColor& color, const MiniMapIconDrawParams& params) const;
	float DrawUnitIcon(TypedRenderBuffer<VA_TYPE_TC3>& rb, size_t iconIdx, const float iconRadius, const float unitRadius, float3 pos, const SColor& color) const;
	void DrawUnitIconScreen(TypedRenderBuffer<VA_TYPE_2DTC3>& rb, size_t iconIdx, const float3& pos, SColor& color, float unitRadius, bool isIcon) const;
};

//TODO remove CUnitDrawerLegacy inheritance
class CUnitDrawerGL4 final : public CUnitDrawerGLSL {
public:
	void Draw(bool drawReflection, bool drawRefraction = false) const override {
		DrawImpl<false, LuaObjType::LUAOBJ_UNIT>(drawReflection, drawRefraction);
	}
	void DrawShadowPass() const override {
		DrawShadowPassImpl<false, LuaObjType::LUAOBJ_UNIT>();
	}

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

	void DrawBuildIcons(const std::vector<CCursorIcons::BuildIcon>& buildIcons) const override;
protected:
	void DrawObjectsShadow(int modelType) const override;
	void DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const override;
	void DrawAlphaObjects(int modelType, bool drawReflection, bool drawRefraction) const override;

	void DrawAlphaObjectsAux(int modelType) const override;
	void DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;
	void DrawAlphaAIUnitBorder(const CUnitDrawerData::TempDrawUnit& unit) const {} //not implemented

	void DrawOpaqueObjectsAux(int modelType) const override;
	void DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const;

	void DrawGhostedBuildings(int modelType) const override;

	void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const;
	void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const;
};

#define unitDrawer (CUnitDrawer::modelDrawer)
