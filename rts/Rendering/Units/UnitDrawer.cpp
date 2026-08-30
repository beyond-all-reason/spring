/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "UnitDrawer.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <map>
#include <vector>

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/UI/MiniMap.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Rendering/Env/IWater.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/GL/glExtra.h"
#include "Rendering/GL/RenderBuffers.h"
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
#include "Rendering/Common/ModelDrawerHelpers.h"
#include "Rendering/Models/3DModelVAO.hpp"
#include "Rendering/Models/ModelsMemStorage.h"

#include "Sim/Features/Feature.h"
#include "Sim/Misc/GlobalConstants.h"
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
#include "System/HashSpec.h"
#include "System/SpringHash.h"

#include "System/Threading/ThreadPool.h"

#include "System/Misc/TracyDefs.h"

CONFIG(int, UnitIconDist).defaultValue(200).headlessValue(0);
CONFIG(float, UnitIconScaleUI).defaultValue(1.0f).minimumValue(0.1f).maximumValue(10.0f);
CONFIG(float, UnitIconFadeStart).defaultValue(3000.0f).minimumValue(1.0f).maximumValue(10000.0f);
CONFIG(float, UnitIconFadeVanish).defaultValue(1000.0f).minimumValue(1.0f).maximumValue(10000.0f);
CONFIG(float, UnitTransparency).defaultValue(0.7f);
CONFIG(bool, UnitIconsAsUI).defaultValue(false).description("Draw unit icons like it is an UI element and not like unit's LOD.");
CONFIG(bool, UnitIconsHideWithUI).defaultValue(false).description("Hide unit icons when UI is hidden.");
CONFIG(float, UnitGhostIconsDimming).defaultValue(0.8).minimumValue(0.0f).maximumValue(1.0f).description("Dimming multiplier for out of radar ghost icons. Setting to 0 disables them.");
CONFIG(bool, UnitIconsSortedByDepth).defaultValue(false).description("Additionally order overlapping unit icons (world, screen and minimap) back-to-front by view depth. Icons always sort by the drawOrder of their icontypes.lua entry when the game defines one; depth ordering on top of that makes overlap stacking change as units and the camera move, hence optional.");

CONFIG(int, MaxDynamicModelLights)
	.defaultValue(1)
	.minimumValue(0);

CONFIG(bool, AdvUnitShading).deprecated(true);

/***********************************************************************/

//don't inherit and leave only static Unit specific helpers
class CUnitDrawerHelper
{
public:
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

	static inline float GetUnitIconScale(const CUnit* unit) {
		const auto& iconData = icon::iconHandler.GetIconData(unit->currentIconIndex);
		float scale = iconData.GetSize();

		if (!minimap->UseUnitIcons())
			return scale;
		if (!iconData.GetRadiusAdjust())
			return scale;

		const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
		const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
		const bool unitVisible = ((losStatus & LOS_INLOS) || ((losStatus & LOS_INRADAR) && ((losStatus & prevMask) == prevMask)));

		if ((unitVisible || gu->spectatingFullView)) {
			scale *= (unit->radius / iconData.GetRadiusScale());
		}

		return scale;
	}
};


/***********************************************************************/


void CUnitDrawer::InitStatic()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CModelDrawerBase<CUnitDrawerData, CUnitDrawer>::InitStatic();

	LuaObjectDrawer::ReadLODScales(LUAOBJ_UNIT);

	CUnitDrawerHelper::LoadUnitExplosionGenerators();

	CUnitDrawer::InitInstance<CUnitDrawerGLSL>(MODEL_DRAWER_GLSL);
	CUnitDrawer::InitInstance<CUnitDrawerGL4 >(MODEL_DRAWER_GL4 );

	SelectImplementation();

	{
		icons2DShader = shaderHandler->CreateProgramObject("[Icons]", "2D");
		icons2DShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/Icons2DVS.glsl", "", GL_VERTEX_SHADER));
		icons2DShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/IconsFS.glsl", "", GL_FRAGMENT_SHADER));
		icons2DShader->BindAttribLocations<VA_TYPE_2DTC3>();
		icons2DShader->Link();

		icons2DShader->Enable();
		icons2DShader->SetUniform("mainTex", 0);
		icons2DShader->SetUniform("custTex", 1);
		icons2DShader->Disable();
		icons2DShader->Validate();
	}
	{
		icons3DShader = shaderHandler->CreateProgramObject("[Icons]", "3D");
		icons3DShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/Icons3DVS.glsl", "", GL_VERTEX_SHADER));
		icons3DShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/IconsFS.glsl", "", GL_FRAGMENT_SHADER));
		icons3DShader->BindAttribLocations<VA_TYPE_TC3>();
		icons3DShader->Link();

		icons3DShader->Enable();
		icons3DShader->SetUniform("mainTex", 0);
		icons3DShader->SetUniform("custTex", 1);
		icons3DShader->Disable();
		icons3DShader->Validate();
	}
}

void CUnitDrawer::KillStatic(bool reload)
{
	CModelDrawerBase<CUnitDrawerData, CUnitDrawer>::KillStatic(reload);

	shaderHandler->ReleaseProgramObjects("[Icons]");
	icons2DShader = nullptr;
	icons3DShader = nullptr;
}

bool CUnitDrawer::ShouldDrawOpaqueUnit(CUnit* u, uint8_t thisPassMask)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (u == ((thisPassMask == DrawFlags::SO_REFLEC_FLAG) ? nullptr : (gu->GetMyPlayer())->fpsController.GetControllee()))
		return false;

	assert(u);
	assert(u->model);

	if (u->drawFlag == 0)
		return false;

	if (u->GetIsIcon())
		return false;

	if (u->HasDrawFlag(DrawFlags::SO_ALPHAF_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_REFLEC_FLAG && !u->HasDrawFlag(DrawFlags::SO_REFLEC_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_REFRAC_FLAG && !u->HasDrawFlag(DrawFlags::SO_REFRAC_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_OPAQUE_FLAG && !u->HasDrawFlag(DrawFlags::SO_OPAQUE_FLAG))
		return false;

	if (LuaObjectDrawer::AddOpaqueMaterialObject(u, LUAOBJ_UNIT))
		return false;

	if ((u->engineDrawMask & thisPassMask) != thisPassMask)
		return false;

	return true;
}

bool CUnitDrawer::ShouldDrawAlphaUnit(CUnit* u, uint8_t thisPassMask)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(u);
	assert(u->model);

	if (u->drawFlag == 0)
		return false;

	if (u->GetIsIcon())
		return false;

	if (u->HasDrawFlag(DrawFlags::SO_OPAQUE_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_REFLEC_FLAG && !u->HasDrawFlag(DrawFlags::SO_REFLEC_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_REFRAC_FLAG && !u->HasDrawFlag(DrawFlags::SO_REFRAC_FLAG))
		return false;

	if (thisPassMask == DrawFlags::SO_ALPHAF_FLAG && !u->HasDrawFlag(DrawFlags::SO_ALPHAF_FLAG))
		return false;

	if (LuaObjectDrawer::AddAlphaMaterialObject(u, LUAOBJ_UNIT))
		return false;

	if ((u->engineDrawMask & thisPassMask) != thisPassMask)
		return false;

	return true;
}

bool CUnitDrawer::ShouldDrawUnitShadow(CUnit* u)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(u);
	assert(u->model);

	static constexpr uint8_t thisPassMask = DrawFlags::SO_SHOPAQ_FLAG;

	if (!u->HasDrawFlag(DrawFlags::SO_SHOPAQ_FLAG))
		return false;

	if (LuaObjectDrawer::AddShadowMaterialObject(u, LUAOBJ_UNIT))
		return false;

	if ((u->engineDrawMask & thisPassMask) != thisPassMask)
		return false;

	return true;
}

/***********************************************************************/


void CUnitDrawerBase::Update() const
{
	SCOPED_TIMER("CUnitDrawerBase::Update");
	modelDrawerData->Update();
}

/***********************************************************************/

CUnitDrawerGLSL::CUnitDrawerGLSL()
{}

CUnitDrawerGLSL::~CUnitDrawerGLSL()
{}

void CUnitDrawerGLSL::DrawUnitModel(const CUnit* unit, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!noLuaCall && unit->luaDraw && eventHandler.DrawUnit(unit))
		return;

	unit->localModel.Draw();
}

void CUnitDrawerGLSL::DrawUnitNoTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
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

void CUnitDrawerGLSL::DrawUnitTrans(const CUnit* unit, uint32_t preList, uint32_t postList, bool lodCall, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glPushMatrix();
	glMultMatrixf(unit->GetTransformMatrix());

	DrawUnitNoTrans(unit, preList, postList, lodCall, noLuaCall);

	glPopMatrix();
}

namespace {
	// icon draw ordering: active whenever the game defines icontypes.lua drawOrder
	// values (games without them skip sorting entirely). The sort runs over flat 8-byte
	// records instead of the payload entries: the icon's drawOrder (higher drawn on
	// top, 16 bits) packs above a path-specific back-to-front depth (full 32 bits),
	// with the payload index in the low 16 bits. Both floats are remapped to
	// order-preserving unsigned bits, so the whole sort is comparator-free integer
	// sorting. The depth field only participates when UnitIconsSortedByDepth is
	// enabled — it makes overlap stacking change as units and the camera move, so by
	// default icons that share a drawOrder keep their stable gather (creation) order
	// via the index tiebreak.
	inline uint32_t SortableFloatBits(float f) {
		uint32_t b;
		std::memcpy(&b, &f, sizeof(b));
		return b ^ (uint32_t(int32_t(b) >> 31) | 0x80000000u);
	}

	// 64k icons per path: covers MAX_UNITS with room to spare for dead ghost buildings
	// (which draw in the same pass); the runtime assert below guards the combined count
	constexpr uint32_t ICON_SORT_IDX_MASK = (1u << 16) - 1;
	static_assert(ICON_SORT_IDX_MASK >= uint32_t(MAX_UNITS), "icon sort records cannot index all units");

	inline uint64_t MakeIconSortRec(float drawOrder, float depth, size_t entryIdx) {
		assert(entryIdx <= ICON_SORT_IDX_MASK);
		// drawOrder gets 16 bits of float precision (plenty for layer indices); depth
		// keeps all 32 so overlap order only changes at true depth crossings instead
		// of jittering at quantization-bucket boundaries as units or the camera move
		return (uint64_t{SortableFloatBits(drawOrder) >> 16} << 48)
		     | (uint64_t{SortableFloatBits(depth)} << 16)
		     | (uint32_t(entryIdx) & ICON_SORT_IDX_MASK);
	}

	// the minimap can be drawn rotated in 90° steps. Overlaps read naturally when the
	// icon nearer the viewer's screen bottom draws on top — the same convention the
	// world view gets from back-to-front depth ordering. Raw pos.z implements that only
	// for the unrotated minimap: flipped 180° it would stack exactly backwards (icons
	// visually behind covering the ones in front of them) and sideways at 90°/270°, so
	// sort by the post-rotation screen vertical instead. A minimap that auto-rotates to
	// follow the camera thereby also matches the world view's stacking for free, while
	// a fixed minimap keeps a stable order instead of reshuffling as the camera turns
	inline float MiniMapIconSortDepth(const float3& pos, int rotation) { // CMiniMap::RotationOptions
		switch (rotation) {
			case CMiniMap::ROTATION_90:  return -pos.x;
			case CMiniMap::ROTATION_180: return -pos.z;
			case CMiniMap::ROTATION_270: return  pos.x;
			default:                     return  pos.z;
		}
	}

	void SortIconRecs(std::vector<uint64_t>& recs) {
		// measured crossover: LSD radix beats std::sort ~3x at 20k records but loses
		// below a few thousand, where its per-pass histogram overhead dominates
		constexpr size_t RADIX_THRESHOLD = 8192;

		const size_t n = recs.size();

		if (n < RADIX_THRESHOLD) {
			std::sort(recs.begin(), recs.end());
			return;
		}

		static std::vector<uint64_t> tmp;
		tmp.resize(n);

		uint64_t* a = recs.data();
		uint64_t* b = tmp.data();

		for (int pass = 0; pass < 8; ++pass) {
			const int shift = pass * 8;

			uint32_t cnt[256] = {0};
			for (size_t i = 0; i < n; ++i)
				++cnt[(a[i] >> shift) & 0xFF];

			// a byte value shared by every key makes this pass an identity permutation;
			// in practice this skips most of the drawOrder half of the key
			bool skip = false;
			for (int d = 0; d < 256; ++d) {
				if (cnt[d] == uint32_t(n)) {
					skip = true;
					break;
				}
			}
			if (skip)
				continue;

			uint32_t pos[256];
			uint32_t sum = 0;
			for (int d = 0; d < 256; ++d) {
				pos[d] = sum;
				sum += cnt[d];
			}

			for (size_t i = 0; i < n; ++i)
				b[pos[(a[i] >> shift) & 0xFF]++] = a[i];

			std::swap(a, b);
		}

		if (a != recs.data())
			std::memcpy(recs.data(), a, n * sizeof(uint64_t));
	}
}

void CUnitDrawerGLSL::DrawUnitMiniMapIcon(TypedRenderBuffer<VA_TYPE_2DTC3>& rb, size_t iconIdx, const float iconScale, const float3& pos, const SColor& color, const MiniMapIconDrawParams& params) const
{
	const float iconSizeX = (iconScale * params.iconSizeX);
	const float iconSizeY = (iconScale * params.iconSizeY);
	float posX = pos.x;
	float posY = pos.z;

	switch (params.rotation) {
		case CMiniMap::ROTATION_90:
			posX = mapDims.mapx * SQUARE_SIZE - posX;

			// Normalize the coordinates to the minimap
			posX = posX / mapDims.mapx * mapDims.mapy;
			posY = posY / mapDims.mapy * mapDims.mapx;

			std::swap(posX, posY);
			break;
		case CMiniMap::ROTATION_180:
			posX = mapDims.mapx * SQUARE_SIZE - posX;
			posY = mapDims.mapy * SQUARE_SIZE - posY;
			break;
		case CMiniMap::ROTATION_270:
			posY = mapDims.mapy * SQUARE_SIZE - posY;

			// Normalize the coordinates to the minimap
			posX = posX / mapDims.mapx * mapDims.mapy;
			posY = posY / mapDims.mapy * mapDims.mapx;

			std::swap(posX, posY);
			break;
		case CMiniMap::ROTATION_0:
			break;
	}

	float x0 = posX - iconSizeX;
	float x1 = posX + iconSizeX;
	float y0 = posY - iconSizeY;
	float y1 = posY + iconSizeY;

	const auto& iconData = icon::iconHandler.GetIconData(iconIdx);
	const auto& tc = iconData.GetTexCoords();
	const float atlasIdx = static_cast<float>(tc.pageNum);

	rb.AddQuadTriangles(
		{ x0, y0, tc.x1, tc.y1, atlasIdx, color },
		{ x1, y0, tc.x2, tc.y1, atlasIdx, color },
		{ x1, y1, tc.x2, tc.y2, atlasIdx, color },
		{ x0, y1, tc.x1, tc.y2, atlasIdx, color }
	);
}

void CUnitDrawerGLSL::DrawUnitMiniMapIcons(const MiniMapIconDrawParams& params) const
{
	ZoneScoped;

	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DTC3>();
	rb.AssertSubmission();

	SColor currentColor;
	const int viewAllyTeam = params.viewAllyTeam;
	const bool isFullView = params.fullView;
	// the local view can use the event-maintained per-unit icon cache; any other
	// perspective recomputes the selection from that viewer's losStatus
	const bool localView = (viewAllyTeam == gu->myAllyTeam && isFullView == gu->spectatingFullView);
	const float ghostIconDimming = modelDrawerData->ghostIconDimming;
	const auto defIconIdx = icon::iconHandler.GetDefaultIconIdx();

	const bool sortDepth = modelDrawerData->sortUnitIconsByDepth;
	const bool sortIcons = sortDepth || icon::iconHandler.HasDrawOrders();
	const auto mmRotation = params.rotation;

	struct IconDrawEntry {
		size_t iconIdx;
		float iconScale;
		float3 pos;
		SColor color;
	};
	static std::vector<IconDrawEntry> entries;
	static std::vector<uint64_t> sortRecs;
	entries.clear();
	sortRecs.clear();
	if (sortIcons) {
		entries.reserve(modelDrawerData->GetUnsortedObjects().size());
		sortRecs.reserve(modelDrawerData->GetUnsortedObjects().size());
	}

	for (auto* unit : modelDrawerData->GetUnsortedObjects()) {
		if (unit->noMinimap)
			continue;

		if (!unit->drawIcon)
			continue;

		if (unit->IsInVoid())
			continue;

		// cull before the icon-selection and color work; for sub-rect views
		// (gl.DrawMiniMapIcons at higher zoom) this skips most units outright
		const float3& pos = (!isFullView) ?
			unit->GetObjDrawErrorPos(viewAllyTeam) :
			unit->GetObjDrawMidPos();

		if (pos.x < params.cullMinX || pos.x > params.cullMaxX || pos.z < params.cullMinZ || pos.z > params.cullMaxZ)
			continue;

		size_t iconIndex = defIconIdx;

		if (params.useIcons)
			iconIndex = localView ? unit->currentIconIndex : modelDrawerData->GetUnitIconIndex(unit, viewAllyTeam, isFullView);

		if (iconIndex == icon::INVALID_ICON_INDEX)
			continue;

		if (params.highlightSelected && unit->isSelected) {
			currentColor = color4::white; // selected color
		}
		else {
			if (params.useSimpleColors) {
				if (unit->team == gu->myTeam) {
					currentColor = params.myColor;
				}
				else if (teamHandler.Ally(viewAllyTeam, unit->allyteam)) {
					currentColor = params.allyColor;
				}
				else {
					currentColor = params.enemyColor;
				}
			}
			else {
				currentColor = teamHandler.Team(unit->team)->color;
			}

			if (!isFullView && !(unit->losStatus[viewAllyTeam] & LOS_INRADAR)) {
				if (ghostIconDimming == 0.0f)
					continue;

				currentColor.r *= ghostIconDimming;
				currentColor.g *= ghostIconDimming;
				currentColor.b *= ghostIconDimming;
			}
		}

		const float iconScale = CUnitDrawerHelper::GetUnitIconScale(unit);

		if (!sortIcons) {
			DrawUnitMiniMapIcon(rb, iconIndex, iconScale, pos, currentColor, params);
		} else {
			entries.push_back({ iconIndex, iconScale, pos, currentColor });
			sortRecs.push_back(MakeIconSortRec(icon::iconHandler.GetIconData(iconIndex).GetDrawOrder(), sortDepth ? MiniMapIconSortDepth(pos, mmRotation) : 0.0f, entries.size() - 1));
		}
	}

	if (!isFullView && ghostIconDimming > 0.0f) {
		for (auto* ghost : modelDrawerData->GetDeadGhostBuildings(viewAllyTeam)) {
			const float3& pos = ghost->midPos;

			if (pos.x < params.cullMinX || pos.x > params.cullMaxX || pos.z < params.cullMinZ || pos.z > params.cullMaxZ)
				continue;

			if (params.useSimpleColors)
				currentColor = params.enemyColor;
			else
				currentColor = teamHandler.Team(ghost->team)->color;

			const size_t iconIndex = params.useIcons ? ghost->currentIconIndex : defIconIdx;

			assert(iconIndex != icon::INVALID_ICON_INDEX);
			if (iconIndex == icon::INVALID_ICON_INDEX)
				continue;

			const auto& iconData = icon::iconHandler.GetIconData(iconIndex);
			const float iconScale = iconData.GetSize();

			currentColor.r *= ghostIconDimming;
			currentColor.g *= ghostIconDimming;
			currentColor.b *= ghostIconDimming;

			if (!sortIcons) {
				DrawUnitMiniMapIcon(rb, iconIndex, iconScale, pos, currentColor, params);
			} else {
				entries.push_back({ iconIndex, iconScale, pos, currentColor });
				sortRecs.push_back(MakeIconSortRec(iconData.GetDrawOrder(), sortDepth ? MiniMapIconSortDepth(pos, mmRotation) : 0.0f, entries.size() - 1));
			}
		}
	}

	if (sortIcons) {
		{
			ZoneScopedN("DrawUnitMiniMapIcons::Sort");
			ZoneValue(uint64_t(sortRecs.size()));
			SortIconRecs(sortRecs);
		}

		ZoneScopedN("DrawUnitMiniMapIcons::Emit");

		for (const uint64_t rec : sortRecs) {
			const auto& e = entries[uint32_t(rec) & ICON_SORT_IDX_MASK];
			DrawUnitMiniMapIcon(rb, e.iconIdx, e.iconScale, e.pos, e.color, params);
		}
	}


	if (!rb.ShouldSubmit())
		return;

	const auto& atlasTexIDs = icon::iconHandler.GetAtlasTextureIDs();
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasTexIDs[0]);
	if (atlasTexIDs[1]) {
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, atlasTexIDs[1]);
	}

	icons2DShader->Enable();
	icons2DShader->SetUniform("alphaCtrl", 0.0f, 1.0f, 0.0f, 0.0f); // GL_GREATER > 0.0

	rb.Submit(GL_TRIANGLES);

	icons2DShader->SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f);
	icons2DShader->Disable();

	if (atlasTexIDs[1])
		glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
}

float CUnitDrawerGLSL::DrawUnitIcon(TypedRenderBuffer<VA_TYPE_TC3>& rb, size_t iconIdx, const float iconRadius, const float unitRadius, float3 pos, const SColor& color) const
{
	const auto& iconData = icon::iconHandler.GetIconData(iconIdx);

	// make sure icon is above ground (needed before we calculate scale below)
	const float h = CGround::GetHeightReal(pos.x, pos.z, false);

	pos.y = std::max(pos.y, h);

	// Calculate the icon size. It scales with:
	//  * The square root of the camera distance.
	//  * The mod defined 'iconSize' (which acts a multiplier).
	//  * The unit radius, depending on whether the mod defined 'radiusadjust' is true or false.
	const float dist = std::min(8000.0f, fastmath::sqrt_builtin(camera->GetPos().SqDistance(pos)));
	const float iconScaleDist = 0.4f * fastmath::sqrt_builtin(dist); // makes far icons bigger
	float scale = iconData.GetSize() * iconScaleDist;

	if (iconData.GetRadiusAdjust() && iconIdx != icon::iconHandler.GetDefaultIconIdx())
		scale *= (unitRadius / iconData.GetRadiusScale());

	// make sure icon is not partly under ground
	pos.y = std::max(pos.y, h + scale);

	const float3 dy = camera->GetUp() * scale;
	const float3 dx = camera->GetRight() * scale;
	const float3 vn = pos - dx;
	const float3 vp = pos + dx;
	const float3 bl = vn - dy; // bottom-left
	const float3 br = vp - dy; // bottom-right
	const float3 tl = vn + dy; // top-left
	const float3 tr = vp + dy; // top-right

	const auto& tc = iconData.GetTexCoords();
	const float atlasIdx = static_cast<float>(tc.pageNum);

	rb.AddQuadTriangles(
		{ tl, tc.x1, tc.y1, atlasIdx, color },
		{ tr, tc.x2, tc.y1, atlasIdx, color },
		{ br, tc.x2, tc.y2, atlasIdx, color },
		{ bl, tc.x1, tc.y2, atlasIdx, color }
	);

	return scale;
}

void CUnitDrawerGLSL::DrawUnitIcons() const
{
	ZoneScoped;
#if 0
	if (game->hideInterface && modelDrawerData->iconHideWithUI)
		return;
#endif

	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_TC3>();
	rb.AssertSubmission();

	const bool sortDepth = modelDrawerData->sortUnitIconsByDepth;
	const bool sortIcons = sortDepth || icon::iconHandler.HasDrawOrders();

	struct IconDrawEntry {
		CUnit* unit;
		float3 pos;
		SColor color;
	};
	static std::vector<IconDrawEntry> entries;
	static std::vector<uint64_t> sortRecs;
	entries.clear();
	sortRecs.clear();
	if (sortIcons) {
		entries.reserve(modelDrawerData->GetUnsortedObjects().size());
		sortRecs.reserve(modelDrawerData->GetUnsortedObjects().size());
	}

	for (auto* unit : modelDrawerData->GetUnsortedObjects()) {
		if (unit->currentIconIndex == icon::INVALID_ICON_INDEX)
			continue;

		if (!unit->GetIsIcon())
			continue;

		if (!unit->drawIcon)
			continue;

		// drawMidPos is auto-calculated now; can wobble on its own as pieces move
		float3 pos = (!gu->spectatingFullView) ?
			unit->GetObjDrawErrorPos(gu->myAllyTeam) :
			unit->GetObjDrawMidPos();

		// use white for selected units
		const auto& iconColor = unit->isSelected ? color4::white : teamHandler.Team(unit->team)->color;

		// negated distance: within equal drawOrder farther icons are drawn first (back-to-front)
		if (!sortIcons) {
			unit->iconRadius = DrawUnitIcon(rb, unit->currentIconIndex, unit->iconRadius, unit->radius, pos, iconColor);
		} else {
			entries.push_back({ unit, pos, iconColor });
			sortRecs.push_back(MakeIconSortRec(icon::iconHandler.GetIconData(unit->currentIconIndex).GetDrawOrder(), sortDepth ? -camera->GetPos().SqDistance(pos) : 0.0f, entries.size() - 1));
		}
	}

	if (sortIcons) {
		{
			ZoneScopedN("DrawUnitIcons::Sort");
			ZoneValue(uint64_t(sortRecs.size()));
			SortIconRecs(sortRecs);
		}

		ZoneScopedN("DrawUnitIcons::Emit");

		for (const uint64_t rec : sortRecs) {
			const auto& e = entries[uint32_t(rec) & ICON_SORT_IDX_MASK];
			e.unit->iconRadius = DrawUnitIcon(rb, e.unit->currentIconIndex, e.unit->iconRadius, e.unit->radius, e.pos, e.color);
		}
	}

	if (!rb.ShouldSubmit())
		return;

	using namespace GL::State;

	auto state = GL::SubState(
		DepthTest(GL_FALSE),
		Blending(GL_FALSE),
		AlphaToCoverage(globalRendering->msaaLevel >= 4 ? GL_TRUE : GL_FALSE)
	);

	const auto& atlasTexIDs = icon::iconHandler.GetAtlasTextureIDs();
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasTexIDs[0]);
	if (atlasTexIDs[1]) {
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, atlasTexIDs[1]);
	}

	icons3DShader->Enable();
	icons3DShader->SetUniform("alphaCtrl", 0.05f, 1.0f, 0.0f, 0.0f); // GL_GREATER > 0.05

	rb.Submit(GL_TRIANGLES);

	icons3DShader->SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f);
	icons3DShader->Disable();

	if (atlasTexIDs[1])
		glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
}

void CUnitDrawerGLSL::DrawUnitIconScreen(TypedRenderBuffer<VA_TYPE_2DTC3>& rb, size_t iconIdx, const float3& pos, SColor& color, float unitRadius, bool isIcon) const
{
	const auto& iconData = icon::iconHandler.GetIconData(iconIdx);

	float unitRadiusMult = iconData.GetSize();
	if (iconData.GetRadiusAdjust() && iconIdx != icon::iconHandler.GetDefaultIconIdx())
		unitRadiusMult *= (unitRadius / iconData.GetRadiusScale());

	unitRadiusMult = unitRadiusMult * 0.75f + 0.25f;

	// fade icons away in high zoom in levels
	if (!isIcon) {
		if (modelDrawerData->iconZoomDist / unitRadiusMult < modelDrawerData->iconFadeVanish)
			return;
		else if (modelDrawerData->iconFadeVanish < modelDrawerData->iconFadeStart && modelDrawerData->iconZoomDist / unitRadiusMult < modelDrawerData->iconFadeStart)
			// alpha range [64, 255], since icons is unrecognisable with alpha < 64
			color.a = 64 + 191.0f * (modelDrawerData->iconZoomDist / unitRadiusMult - modelDrawerData->iconFadeVanish) / (modelDrawerData->iconFadeStart - modelDrawerData->iconFadeVanish);
	}

	// calculate the vertices
	const float offset = modelDrawerData->iconSizeBase / 2.0f * unitRadiusMult;

	const float x0 = (pos.x - offset) / globalRendering->viewSizeX;
	const float y0 = (pos.y + offset) / globalRendering->viewSizeY;
	const float x1 = (pos.x + offset) / globalRendering->viewSizeX;
	const float y1 = (pos.y - offset) / globalRendering->viewSizeY;

	if (x1 < 0 && x0 > 1 && y0 < 0 && y1 > 1)
		return; // don't try to draw when totally outside the screen

	const auto& tc = iconData.GetTexCoords();
	const float atlasIdx = static_cast<float>(tc.pageNum);

	rb.AddQuadTriangles(
		{ x0, y0, tc.x1, tc.y1, atlasIdx, color },
		{ x1, y0, tc.x2, tc.y1, atlasIdx, color },
		{ x1, y1, tc.x2, tc.y2, atlasIdx, color },
		{ x0, y1, tc.x1, tc.y2, atlasIdx, color }
	);
}

void CUnitDrawerGLSL::DrawUnitIconsScreen() const
{
	ZoneScoped;

	if (game->hideInterface && modelDrawerData->iconHideWithUI)
		return;

	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2DTC3>();
	rb.AssertSubmission();

	SColor currentColor;
	const auto myAllyTeam = gu->myAllyTeam;
	const auto isFullView = gu->spectatingFullView;
	const float ghostIconDimming = modelDrawerData->ghostIconDimming;

	const bool sortDepth = modelDrawerData->sortUnitIconsByDepth;
	const bool sortIcons = sortDepth || icon::iconHandler.HasDrawOrders();

	struct IconDrawEntry {
		size_t iconIdx;
		float3 pos;
		SColor color;
		float radius;
		bool isIcon;
	};
	static std::vector<IconDrawEntry> entries;
	static std::vector<uint64_t> sortRecs;
	entries.clear();
	sortRecs.clear();
	if (sortIcons) {
		entries.reserve(modelDrawerData->GetUnsortedObjects().size());
		sortRecs.reserve(modelDrawerData->GetUnsortedObjects().size());
	}

	for (auto* unit : modelDrawerData->GetUnsortedObjects()) {
		if (unit->currentIconIndex == icon::INVALID_ICON_INDEX)
			continue;

		if (!unit->drawIcon)
			continue;

		// needed?
		const bool canSee = gu->spectatingFullView || (unit->losStatus[gu->myAllyTeam] && (LOS_INLOS | LOS_CONTRADAR | LOS_PREVLOS) == (LOS_INLOS | LOS_CONTRADAR | LOS_PREVLOS));
		if (!canSee)
			continue;

		assert(!unit->IsInVoid());


		// drawMidPos is auto-calculated now; can wobble on its own as pieces move
		const float3 worldPos = (!isFullView) ?
			unit->GetObjDrawErrorPos(myAllyTeam) :
			unit->GetObjDrawMidPos();

		float3 pos = camera->CalcViewPortCoordinates(worldPos);
		if (pos.z > 1.0f || pos.z < 0.0f)
			continue;

		if (unit->isSelected) {
			currentColor = color4::white; // selected color
		}
		else {
			currentColor = teamHandler.Team(unit->team)->color;
			if (!isFullView && !(unit->losStatus[myAllyTeam] & LOS_INRADAR)) {
				if (ghostIconDimming == 0.0f)
					continue;

				currentColor.r *= ghostIconDimming;
				currentColor.g *= ghostIconDimming;
				currentColor.b *= ghostIconDimming;
			}
		}

		if (!sortIcons) {
			DrawUnitIconScreen(rb, unit->currentIconIndex, pos, currentColor, unit->radius, unit->GetIsIcon());
		} else {
			entries.push_back({ unit->currentIconIndex, pos, currentColor, unit->radius, unit->GetIsIcon() });
			// negated camera distance: farther icons draw first (back-to-front). The
			// post-projection viewport z is unusable here — it compresses everything
			// toward 1.0, which the 16-bit key quantization collapses into one value
			sortRecs.push_back(MakeIconSortRec(icon::iconHandler.GetIconData(unit->currentIconIndex).GetDrawOrder(), sortDepth ? -camera->GetPos().SqDistance(worldPos) : 0.0f, entries.size() - 1));
		}
	}

	if (!isFullView && ghostIconDimming > 0.0f) {
		for (auto* ghost : modelDrawerData->GetDeadGhostBuildings(gu->myAllyTeam)) {
			float3 pos = camera->CalcViewPortCoordinates(ghost->midPos);
			if (pos.z > 1.0f || pos.z < 0.0f)
				continue;

			const auto& iconIndex = ghost->currentIconIndex;

			assert(iconIndex != icon::INVALID_ICON_INDEX);
			if (iconIndex == icon::INVALID_ICON_INDEX)
				continue;

			currentColor = teamHandler.Team(ghost->team)->color;
			currentColor.r *= ghostIconDimming;
			currentColor.g *= ghostIconDimming;
			currentColor.b *= ghostIconDimming;

			if (!sortIcons) {
				DrawUnitIconScreen(rb, iconIndex, pos, currentColor, ghost->radius, false);
			} else {
				entries.push_back({ iconIndex, pos, currentColor, ghost->radius, false });
				sortRecs.push_back(MakeIconSortRec(icon::iconHandler.GetIconData(iconIndex).GetDrawOrder(), sortDepth ? -camera->GetPos().SqDistance(ghost->midPos) : 0.0f, entries.size() - 1));
			}
		}
	}

	if (sortIcons) {
		{
			ZoneScopedN("DrawUnitIconsScreen::Sort");
			ZoneValue(uint64_t(sortRecs.size()));
			SortIconRecs(sortRecs);
		}

		ZoneScopedN("DrawUnitIconsScreen::Emit");

		for (const uint64_t rec : sortRecs) {
			auto& e = entries[uint32_t(rec) & ICON_SORT_IDX_MASK];
			DrawUnitIconScreen(rb, e.iconIdx, e.pos, e.color, e.radius, e.isIcon);
		}
	}

	if (!rb.ShouldSubmit())
		return;

	using namespace GL::State;

	auto state = GL::SubState(
		DepthTest(GL_FALSE),
		Blending(GL_TRUE),
		BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
	);

	const auto& atlasTexIDs = icon::iconHandler.GetAtlasTextureIDs();
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, atlasTexIDs[0]);
	if (atlasTexIDs[1]) {
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, atlasTexIDs[1]);
	}

	icons3DShader->Enable();
	icons3DShader->SetUniform("alphaCtrl", 0.05f, 1.0f, 0.0f, 0.0f); // GL_GREATER > 0.05

	rb.Submit(GL_TRIANGLES);

	icons3DShader->SetUniform("alphaCtrl", 0.0f, 0.0f, 0.0f, 1.0f);
	icons3DShader->Disable();

	if (atlasTexIDs[1])
		glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
}

void CUnitDrawerGLSL::DrawObjectsShadow(int modelType) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));
		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawUnitShadow(o);
		}

		CModelDrawerHelper::modelDrawerHelpers[modelType]->UnbindShadowTex();
	}
}

void CUnitDrawerGLSL::DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const uint8_t thisPassMask =
		(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_OPAQUE_FLAG +
		(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
		(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawOpaqueUnit(o, thisPassMask);
		}
	}
}

void CUnitDrawerGLSL::DrawAlphaObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const uint8_t thisPassMask =
		(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_ALPHAF_FLAG +
		(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
		(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			DrawAlphaUnit(o, thisPassMask);
		}
	}

	// living and dead ghosted buildings
	if (!gu->spectatingFullView)
		DrawGhostedBuildings(modelType);
}

void CUnitDrawerGLSL::DrawOpaqueObjectsAux(int modelType) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpOpaqueUnits = modelDrawerData->GetTempOpaqueDrawUnits(modelType);

	// NOTE: not type-sorted
	for (const auto& unit : tmpOpaqueUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawOpaqueAIUnit(unit);
	}
}

void CUnitDrawerGLSL::DrawAlphaObjectsAux(int modelType) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpAlphaUnits = modelDrawerData->GetTempAlphaDrawUnits(modelType);

	// NOTE: not type-sorted
	for (const auto& unit : tmpAlphaUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawAlphaAIUnit(unit);
		DrawAlphaAIUnitBorder(unit);
	}
}

void CUnitDrawerGLSL::DrawGhostedBuildings(int modelType) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto& deadGhostedBuildings = modelDrawerData->GetDeadGhostBuildings(gu->myAllyTeam, modelType);
	const auto& liveGhostedBuildings = modelDrawerData->GetLiveGhostBuildings(gu->myAllyTeam, modelType);

	glColor4f(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);

	// buildings that died while ghosted
	for (const GhostSolidObject* dgb : deadGhostedBuildings) {
		const S3DModel* model = dgb->GetModel();
		if (!camera->InView(dgb->pos, model->GetDrawRadius()))
			continue;

		glPushMatrix();
		glTranslatef3(dgb->pos);
		glRotatef(dgb->facing * 90.0f, 0, 1, 0);

		CModelDrawerHelper::BindModelTypeTexture(modelType, model->textureType);
		SetTeamColor(dgb->team, IModelDrawerState::alphaValues.y);

		model->DrawStatic();
		glPopMatrix();
	}

	// buildings that left LOS but are still alive
	for (const auto& lgb : liveGhostedBuildings) {
		const CUnit* unit = lgb.unit;

		// check for decoy models
		const UnitDef* decoyDef = unit->unitDef->decoyDef;
		const S3DModel* model = (decoyDef == nullptr) ? unit->model : decoyDef->LoadModel();

		// FIXME: needs a second pass
		if (model->type != modelType)
			continue;

		const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

		// ghosted enemy units
		if (losStatus & LOS_CONTRADAR) {
			glColor4f(0.9f, 0.9f, 0.9f, IModelDrawerState::alphaValues.z);
		}
		else {
			glColor4f(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);
		}

		glPushMatrix();
		glTranslatef3(unit->drawPos);
		glRotatef(unit->buildFacing * 90.0f, 0, 1, 0);

		// the units in liveGhostedBuildings[modelType] are not
		// sorted by textureType, but we cannot merge them with
		// alphaModelRenderers[modelType] either since they are
		// not actually cloaked
		CModelDrawerHelper::BindModelTypeTexture(modelType, model->textureType);

		// color with the team the unit was last seen under, not the live unit's current team
		const float ghostAlpha = (losStatus & LOS_CONTRADAR) ? IModelDrawerState::alphaValues.z : IModelDrawerState::alphaValues.y;
		SetTeamColor(lgb.team, ghostAlpha);
		model->DrawStatic();
		glPopMatrix();

		glColor4f(1.0f, 1.0f, 1.0f, IModelDrawerState::alphaValues.x);
	}
}

void CUnitDrawerGLSL::DrawOpaqueUnit(CUnit* unit, uint8_t thisPassMask) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!ShouldDrawOpaqueUnit(unit, thisPassMask))
		return;

	// draw the unit with the default (non-Lua) material
	SetTeamColor(unit->team);
	DrawUnitTrans(unit, 0, 0, false, false);
}

void CUnitDrawerGLSL::DrawUnitShadow(CUnit* unit) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (ShouldDrawUnitShadow(unit))
		DrawUnitTrans(unit, 0, 0, false, false);
}

void CUnitDrawerGLSL::DrawAlphaUnit(CUnit* unit, uint8_t thisPassMask) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!ShouldDrawAlphaUnit(unit, thisPassMask))
		return;

	if (unit->GetIsIcon())
		return;

	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];

	if ((losStatus & LOS_INLOS) || gu->spectatingFullView) {
		SetTeamColor(unit->team, IModelDrawerState::alphaValues.x);
		DrawUnitTrans(unit, 0, 0, false, false);
	}
}

void CUnitDrawerGLSL::DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glPushMatrix();
	glTranslatef3(unit.pos);
	glRotatef(unit.rotation * math::RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

	const UnitDef* def = unit.GetUnitDef();
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	CModelDrawerHelper::BindModelTypeTexture(mdl->type, mdl->textureType);
	SetTeamColor(unit.team);
	mdl->DrawStatic();

	glPopMatrix();
}

void CUnitDrawerGLSL::DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glPushMatrix();
	glTranslatef3(unit.pos);
	glRotatef(unit.rotation * math::RAD_TO_DEG, 0.0f, 1.0f, 0.0f);

	const UnitDef* def = unit.GetUnitDef();
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	CModelDrawerHelper::BindModelTypeTexture(mdl->type, mdl->textureType);
	SetTeamColor(unit.team, IModelDrawerState::alphaValues.x);
	mdl->DrawStatic();

	glPopMatrix();
}

void CUnitDrawerGLSL::DrawAlphaAIUnitBorder(const CUnitDrawerData::TempDrawUnit& unit) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!unit.drawBorder)
		return;

	SetTeamColor(unit.team, IModelDrawerState::alphaValues.w);

	const BuildInfo buildInfo(unit.GetUnitDef(), unit.pos, unit.facing);
	const float3 buildPos = CGameHelper::Pos2BuildPos(buildInfo, false);

	const float xsize = buildInfo.GetXSize() * (SQUARE_SIZE >> 1);
	const float zsize = buildInfo.GetZSize() * (SQUARE_SIZE >> 1);

	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();

	const SColor col = SColor{ 0.2f, 1.0f, 0.2f, IModelDrawerState::alphaValues.w };

	rb.AddVertices({
		{buildPos + float3( xsize, 1.0f,  zsize), col},
		{buildPos + float3(-xsize, 1.0f,  zsize), col},
		{buildPos + float3(-xsize, 1.0f, -zsize), col},
		{buildPos + float3( xsize, 1.0f, -zsize), col},
		{buildPos + float3( xsize, 1.0f,  zsize), col}
	});

	GLint progID = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &progID);

	auto& sh = rb.GetShader();
	sh.Enable();
	rb.DrawArrays(GL_LINE_STRIP);
	sh.Disable();

	if (progID > 0)
		glUseProgram(progID);

	glColor4f(1.0f, 1.0f, 1.0f, IModelDrawerState::alphaValues.x);
	glEnable(GL_TEXTURE_2D);
}

void CUnitDrawerGLSL::DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 stageBounds = { 0.0f, unit->model->CalcDrawHeight(), unit->buildProgress };

	// draw-height defaults to maxs.y - mins.y, but can be overridden for non-3DO models
	// the default value derives from the model vertices and makes more sense to use here
	//
	// Both clip planes move up. Clip plane 0 is the upper bound of the model,
	// clip plane 1 is the lower bound. In other words, clip plane 0 makes the
	// wireframe/flat color/texture appear, and clip plane 1 then erases the
	// wireframe/flat color later on.
	const double upperPlanes[BuildStages::BUILDSTAGE_CNT][4] = {
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f       )},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const double lowerPlanes[BuildStages::BUILDSTAGE_CNT][4] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 3.0f  - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                                           0.0f },
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_CURRENT_BIT);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	{
		// wireframe, unconditional
		DrawModelWireBuildStageShadow(unit, upperPlanes[BUILDSTAGE_WIRE], lowerPlanes[BUILDSTAGE_WIRE], noLuaCall);
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

void CUnitDrawerGLSL::DrawModelWireBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (globalRendering->amdHacks) {
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

	if (globalRendering->amdHacks) {
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
	}
}

void CUnitDrawerGLSL::DrawModelFlatBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glPushMatrix();
	glLoadIdentity();
	glClipPlane(GL_CLIP_PLANE0, upperPlane);
	glClipPlane(GL_CLIP_PLANE1, lowerPlane);
	glPopMatrix();

	DrawUnitModel(unit, noLuaCall);
}

void CUnitDrawerGLSL::DrawModelFillBuildStageShadow(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	DrawUnitModel(unit, noLuaCall);
}

void CUnitDrawerGLSL::DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
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
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f       )},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const double lowerPlanes[4][4] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z *  3.0f - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                                           0.0f },
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_CURRENT_BIT);
	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	{
		// wireframe, unconditional
		SetNanoColor(float4(stageColors[0] * wireColorMult, 1.0f));
		DrawModelWireBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_WIRE], lowerPlanes[BUILDSTAGE_WIRE], noLuaCall);
	}

	if (stageBounds.z > 1.0f / 3.0f) {
		// flat-colored, conditional
		SetNanoColor(float4(stageColors[1] * flatColorMult, 1.0f));
		DrawModelFlatBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_WIRE], lowerPlanes[BUILDSTAGE_WIRE], noLuaCall);
	}

	glDisable(GL_CLIP_PLANE1);

	if (stageBounds.z > 2.0f / 3.0f) {
		// fully-shaded, conditional
		SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f));
		DrawModelFillBuildStageOpaque(unit, upperPlanes[BUILDSTAGE_FILL], lowerPlanes[BUILDSTAGE_FILL], noLuaCall);
	}

	SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f)); // turn off in any case
	glDisable(GL_CLIP_PLANE0);
	glPopAttrib();
}

void CUnitDrawerGLSL::DrawModelWireBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (globalRendering->amdHacks) {
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
	} else {
		glClipPlane(GL_CLIP_PLANE0, upperPlane);
		glClipPlane(GL_CLIP_PLANE1, lowerPlane);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	DrawUnitModel(unit, noLuaCall);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if (globalRendering->amdHacks) {
		glEnable(GL_CLIP_PLANE0);
		glEnable(GL_CLIP_PLANE1);
	}
}

void CUnitDrawerGLSL::DrawModelFlatBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	glClipPlane(GL_CLIP_PLANE0, upperPlane);
	glClipPlane(GL_CLIP_PLANE1, lowerPlane);

	DrawUnitModel(unit, noLuaCall);
}

void CUnitDrawerGLSL::DrawModelFillBuildStageOpaque(const CUnit* unit, const double* upperPlane, const double* lowerPlane, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (globalRendering->amdHacks)
		glDisable(GL_CLIP_PLANE0);
	else
		glClipPlane(GL_CLIP_PLANE0, upperPlane);

	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	DrawUnitModel(unit, noLuaCall);
	glDisable(GL_POLYGON_OFFSET_FILL);
}

void CUnitDrawerGLSL::PushIndividualOpaqueState(const CUnit* unit, bool deferredPass) const { PushIndividualOpaqueState(unit->model, unit->team, deferredPass); }
void CUnitDrawerGLSL::PushIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// these are not handled by Setup*Drawing but CGame
	// easier to assume they no longer have the correct
	// values at this point
	glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	SetupOpaqueDrawing(deferredPass);
	CModelDrawerHelper::PushModelRenderState(model);
	SetTeamColor(teamID);
}

void CUnitDrawerGLSL::PushIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	SetupAlphaDrawing(deferredPass);
	CModelDrawerHelper::PushModelRenderState(model);
	SetTeamColor(teamID, IModelDrawerState::alphaValues.x);
}

void CUnitDrawerGLSL::PopIndividualOpaqueState(const CUnit* unit, bool deferredPass) const { PopIndividualOpaqueState(unit->model, unit->team, deferredPass); }
void CUnitDrawerGLSL::PopIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CModelDrawerHelper::PopModelRenderState(model);
	ResetOpaqueDrawing(deferredPass);

	glPopAttrib();
}

void CUnitDrawerGLSL::PopIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CModelDrawerHelper::PopModelRenderState(model);
	ResetAlphaDrawing(deferredPass);
}

void CUnitDrawerGLSL::DrawIndividual(const CUnit* unit, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (LuaObjectDrawer::DrawSingleObject(unit, LUAOBJ_UNIT /*, noLuaCall*/))
		return;

	// set the full default state
	PushIndividualOpaqueState(unit, false);
	DrawUnitTrans(unit, 0, 0, false, noLuaCall);
	PopIndividualOpaqueState(unit, false);
}

void CUnitDrawerGLSL::DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (LuaObjectDrawer::DrawSingleObjectNoTrans(unit, LUAOBJ_UNIT /*, noLuaCall*/))
		return;

	PushIndividualOpaqueState(unit, false);
	DrawUnitNoTrans(unit, 0, 0, false, noLuaCall);
	PopIndividualOpaqueState(unit, false);
}

void CUnitDrawerGLSL::DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const S3DModel* model = objectDef->LoadModel();

	if (model == nullptr)
		return;

	if (!rawState) {
		if (!CModelDrawerHelper::DIDCheckMatrixMode(GL_MODELVIEW))
			return;

		// teamID validity is checked by SetTeamColor
		PushIndividualOpaqueState(model, teamID, false);

		// NOTE:
		//   unlike DrawIndividual(...) the model transform is
		//   always provided by Lua, not taken from the object
		//   (which does not exist here) so we must restore it
		//   (by undoing the UnitDrawerState MVP setup)
		//
		//   assumes the Lua transform includes a LoadIdentity!
		CModelDrawerHelper::DIDResetPrevProjection(toScreen);
		CModelDrawerHelper::DIDResetPrevModelView();
	}

	model->DrawStatic();

	if (!rawState) {
		PopIndividualOpaqueState(model, teamID, false);
	}
}

void CUnitDrawerGLSL::DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const S3DModel* model = objectDef->LoadModel();

	if (model == nullptr)
		return;

	if (!rawState) {
		if (!CModelDrawerHelper::DIDCheckMatrixMode(GL_MODELVIEW))
			return;

		PushIndividualAlphaState(model, teamID, false);

		CModelDrawerHelper::DIDResetPrevProjection(toScreen);
		CModelDrawerHelper::DIDResetPrevModelView();
	}

	model->DrawStatic();

	if (!rawState) {
		PopIndividualAlphaState(model, teamID, false);
	}
}

bool CUnitDrawerGLSL::ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands) const
{
	RECOIL_DETAILED_TRACY_ZONE;

	CFeature* feature = nullptr;

	std::vector<uint8_t> statuses;

	struct BuildCache {
		uint64_t key;
		int unitDefID;
		int createFrame;
		bool canBuild;
		std::vector<uint8_t> statuses;
	};

	static std::vector<BuildCache> buildCache;

	const float3& pos = buildInfo.pos;

	uint64_t hashKey = spring::LiteHash(pos);
	hashKey = spring::hash_combine(spring::LiteHash(buildInfo.buildFacing), hashKey);
	// TestUnitBuildSquare statuses also depend on the definition and queued
	// commands. Both the world and minimap passes share this cache.
	hashKey = spring::hash_combine(spring::LiteHash(buildInfo.def->id), hashKey);
	for (const Command& command: commands) {
		hashKey = spring::hash_combine(spring::LiteHash(command.GetID()), hashKey);
		for (unsigned int i = 0; i < command.GetNumParams(); ++i)
			hashKey = spring::hash_combine(spring::LiteHash(command.GetParam(i)), hashKey);
	}

	static constexpr int CACHE_VALIDITY_PERIOD = GAME_SPEED / 5;
	std::erase_if(buildCache, [](const BuildCache& bc) {
		return gs->frameNum - bc.createFrame >= CACHE_VALIDITY_PERIOD;
	});

	const int x1 = pos.x - (buildInfo.GetXSize() * 0.5f * SQUARE_SIZE);
	const int x2 = x1 + (buildInfo.GetXSize() * SQUARE_SIZE);
	const int z1 = pos.z - (buildInfo.GetZSize() * 0.5f * SQUARE_SIZE);
	const int z2 = z1 + (buildInfo.GetZSize() * SQUARE_SIZE);
	const float h = CGameHelper::GetBuildHeight(pos, buildInfo.def, false);

	bool canBuild;

	// A hash collision may reuse incorrect statuses. Verify the UnitDef separately
	// to prevent an OOB read from a mismatched footprint; collisions within the
	// same UnitDef may still render incorrect status data.
	const auto it = std::find_if(buildCache.begin(), buildCache.end(), [hashKey, &buildInfo](const BuildCache& bc) {
		return (
			bc.key == hashKey &&
			bc.unitDefID == buildInfo.def->id
		);
	});
	if (it != buildCache.end()) {
		statuses = it->statuses;
		canBuild = it->canBuild;
	}
	else {
		canBuild = !!CGameHelper::TestUnitBuildSquare(
			buildInfo,
			feature,
			-1,
			false,
			&statuses,
			&commands
		);
		buildCache.emplace_back();
		auto& buildCacheItem = buildCache.back();

		buildCacheItem.key = hashKey;
		buildCacheItem.unitDefID = buildInfo.def->id;
		buildCacheItem.canBuild = canBuild;
		buildCacheItem.createFrame = gs->frameNum;
		buildCacheItem.statuses = statuses;
	}

	eventHandler.DrawBuildSquare(
		buildInfo.def->id,
		static_cast<int>(pos.x),
		static_cast<int>(pos.z),
		buildInfo.buildFacing,
		statuses
	);

	if (!CUnitDrawer::EngineBuildSquareRendering()) {
		return canBuild;
	}

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);

	static constexpr SColor buildColorT  = { 0.0f, 0.9f, 0.0f, 0.7f };
	static constexpr SColor buildColorF  = { 0.9f, 0.8f, 0.0f, 0.7f };
	static constexpr SColor featureColor = { 0.9f, 0.8f, 0.0f, 0.7f };
	static constexpr SColor illegalColor = { 0.9f, 0.0f, 0.0f, 0.7f };

	static auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_C>();
	rb.AssertSubmission();

	auto& sh = rb.GetShader();

	sh.Enable();

	const auto* buildColor = canBuild ? &buildColorT : &buildColorF;
	const int numX = buildInfo.GetXSize();
	const int numZ = buildInfo.GetZSize();
	const int sx1 = int(pos.x / SQUARE_SIZE) - (numX >> 1);
	const int sz1 = int(pos.z / SQUARE_SIZE) - (numZ >> 1);

	for (int zi = 0; zi < numZ; zi++) {
		for (int xi = 0; xi < numX; xi++) {
			const auto status = static_cast<CGameHelper::BuildSquareStatus>(statuses[zi * numX + xi]);
			const float3 sqrPos = {
				static_cast<float>((sx1 + xi) * SQUARE_SIZE),
				h,
				static_cast<float>((sz1 + zi) * SQUARE_SIZE)
			};
			const SColor* color = nullptr;
			switch (status) {
				case CGameHelper::BUILDSQUARE_OPEN:
					color = buildColor;
					break;
				case CGameHelper::BUILDSQUARE_OCCUPIED:
				case CGameHelper::BUILDSQUARE_RECLAIMABLE:
					color = &featureColor;
					break;
				default:
					color = &illegalColor;
					break;
			}
			rb.AddQuadLines(
				{ sqrPos                                      , *color },
				{ sqrPos + float3(SQUARE_SIZE, 0, 0          ), *color },
				{ sqrPos + float3(SQUARE_SIZE, 0, SQUARE_SIZE), *color },
				{ sqrPos + float3(0          , 0, SQUARE_SIZE), *color }
			);
		}
	}
	rb.Submit(GL_LINES);

	if (h < 0.0f) {
		constexpr SColor s = { 0,   0, 255, 128 };
		constexpr SColor e = { 0, 128, 255, 255 };

		rb.AddVertex({ float3(x1, h, z1), s }); rb.AddVertex({ float3(x1, 0.f, z1), e });
		rb.AddVertex({ float3(x1, h, z2), s }); rb.AddVertex({ float3(x1, 0.f, z2), e });
		rb.AddVertex({ float3(x2, h, z2), s }); rb.AddVertex({ float3(x2, 0.f, z2), e });
		rb.AddVertex({ float3(x2, h, z1), s }); rb.AddVertex({ float3(x2, 0.f, z1), e });
		rb.Submit(GL_LINES);

		rb.AddVertex({ float3(x1, 0.0f, z1), e });
		rb.AddVertex({ float3(x1, 0.0f, z2), e });
		rb.AddVertex({ float3(x2, 0.0f, z2), e });
		rb.AddVertex({ float3(x2, 0.0f, z1), e });
		rb.Submit(GL_LINE_LOOP);
	}

	sh.Disable();


	glEnable(GL_DEPTH_TEST);

	return canBuild;
}

void CUnitDrawerGLSL::DrawBuildIcons(const std::vector<CCursorIcons::BuildIcon>& buildIcons) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (buildIcons.empty())
		return;

	glEnable(GL_DEPTH_TEST);
	glColor4f(1.0f, 1.0f, 1.0f, 0.3f);

	for (const auto& buildIcon : buildIcons) {
		const auto* unitDef = unitDefHandler->GetUnitDefByID(-(buildIcon.cmd));
		assert(unitDef);

		const auto* model = unitDef->LoadModel();
		assert(model);

		if (!camera->InView(buildIcon.pos, model->GetDrawRadius()))
			continue;

		glPushMatrix();
		glLoadIdentity();
		glTranslatef3(buildIcon.pos);
		glRotatef(buildIcon.facing * 90.0f, 0.0f, 1.0f, 0.0f);

		unitDrawer->DrawIndividualDefAlpha(unitDef, buildIcon.team, false);

		glPopMatrix();
	}

	glDisable(GL_DEPTH_TEST);
}

/***********************************************************************/

// CUnitDrawerGLSL::DrawBuildIcons is seemingly unbeatable in terms of FPS ?
void CUnitDrawerGL4::DrawBuildIcons(const std::vector<CCursorIcons::BuildIcon>& buildIcons) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (buildIcons.empty())
		return;

	glEnable(GL_DEPTH_TEST);
	SetupAlphaDrawing(false);

	const auto oldMM = modelDrawerState->SetMatrixMode(ShaderMatrixModes::STATIC_MATMODE);
	const auto oldSM = modelDrawerState->SetShadingMode(ShaderShadingModes::SKIP_SHADING);
	modelDrawerState->SetTeamColor(0);
	modelDrawerState->SetColorMultiplier(0.9f, 0.9f, 0.9f, 0.3f);

	int prevModelType = -1;
	int prevTexType = -1;

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (const auto& buildIcon : buildIcons) {
		const auto* unitDef = unitDefHandler->GetUnitDefByID(-(buildIcon.cmd));
		assert(unitDef);

		const auto* model = unitDef->LoadModel();
		assert(model);

		if (!camera->InView(buildIcon.pos, model->GetDrawRadius()))
			continue;

		CMatrix44f staticWorldMat;

		staticWorldMat.Translate(buildIcon.pos);
		staticWorldMat.RotateY(-buildIcon.facing * math::DEG_TO_RAD * 90.0f);

		modelDrawerState->SetStaticModelMatrix(staticWorldMat);

		if (prevModelType != model->type || prevTexType != model->textureType) {
			if (prevModelType != -1)
				CModelDrawerHelper::PopModelRenderState(prevModelType);

			prevModelType = model->type; prevTexType = model->textureType;
			CModelDrawerHelper::PushModelRenderState(model->type);
			CModelDrawerHelper::BindModelTypeTexture(model->type, model->textureType); //inefficient rendering, but w/e
		}

		smv.SubmitImmediately(model, static_cast<uint16_t>(buildIcon.team));
	}

	if (prevModelType != -1)
		CModelDrawerHelper::PopModelRenderState(prevModelType);

	modelDrawerState->SetColorMultiplier();
	modelDrawerState->SetMatrixMode(oldMM);
	modelDrawerState->SetShadingMode(oldSM);

	smv.Unbind();

	ResetAlphaDrawing(false);
	glDisable(GL_DEPTH_TEST);
}


void CUnitDrawerGL4::DrawObjectsShadow(int modelType) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));
		const auto& bin = mdlRenderer.GetObjectBin(i);

		static vector<const ObjType*> beingBuilt;
		beingBuilt.clear();

		for (auto* o : bin) {
			if (!ShouldDrawUnitShadow(o))
				continue;

			if (o->beingBuilt && o->unitDef->showNanoFrame) {
				beingBuilt.emplace_back(o);
				continue;
			}

			smv.AddToSubmission(o);
		}

		smv.Submit(GL_TRIANGLES, false);

		for (auto* o : beingBuilt) {
			DrawUnitModelBeingBuiltShadow(o, false);
		}

		CModelDrawerHelper::modelDrawerHelpers[modelType]->UnbindShadowTex();
	}

	smv.Unbind();
}

void CUnitDrawerGL4::DrawOpaqueObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const uint8_t thisPassMask =
		(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_OPAQUE_FLAG +
		(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
		(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	SetTeamColor(0, 1.0f);
	modelDrawerState->SetColorMultiplier();

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	for (unsigned int i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		static vector<const ObjType*> beingBuilt;
		beingBuilt.clear();

		for (auto* o : mdlRenderer.GetObjectBin(i)) {
			if (!ShouldDrawOpaqueUnit(o, thisPassMask))
				continue;

			if (o->beingBuilt && o->unitDef->showNanoFrame) {
				beingBuilt.emplace_back(o);
				continue;
			}

			smv.AddToSubmission(o);
		}

		smv.Submit(GL_TRIANGLES, false);

		for (auto* o : beingBuilt) {
			DrawUnitModelBeingBuiltOpaque(o, false);
		}
	}

	smv.Unbind();
}

void CUnitDrawerGL4::DrawAlphaObjects(int modelType, bool drawReflection, bool drawRefraction) const
{
	const uint8_t thisPassMask =
		(1 - (drawReflection || drawRefraction)) * DrawFlags::SO_ALPHAF_FLAG +
		(drawReflection * DrawFlags::SO_REFLEC_FLAG) +
		(drawRefraction * DrawFlags::SO_REFRAC_FLAG);

	const auto& mdlRenderer = modelDrawerData->GetModelRenderer(modelType);

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	//some magical constant that equalizes alpha with GLSL drawer, the origin of this difference is unknown
	modelDrawerState->SetColorMultiplier(0.6f);
	modelDrawerState->SetTeamColor(0, IModelDrawerState::alphaValues.x); //teamID doesn't matter here
	//main cloaked alpha pass
	for (uint32_t i = 0, n = mdlRenderer.GetNumObjectBins(); i < n; i++) {
		if (mdlRenderer.GetObjectBin(i).empty())
			continue;

		CModelDrawerHelper::BindModelTypeTexture(modelType, mdlRenderer.GetObjectBinKey(i));

		const auto& bin = mdlRenderer.GetObjectBin(i);

		for (auto* o : bin) {
			if (!ShouldDrawAlphaUnit(o, thisPassMask))
				continue;

			smv.AddToSubmission(o);
		}

		smv.Submit(GL_TRIANGLES, false);
	}

	smv.Unbind();

	// living and dead ghosted buildings
	if (!gu->spectatingFullView)
		DrawGhostedBuildings(modelType);
}

void CUnitDrawerGL4::DrawGhostedBuildings(int modelType) const
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	// Ghost buildings are static (no animation, never move), so each gets a single world-transform
	// slot in the transforms SSBO and is drawn batched through ARRAY_MATMODE - one multidraw per
	// (color bucket x texture type) instead of one immediate draw per ghost.
	const auto oldMM = modelDrawerState->SetMatrixMode(ShaderMatrixModes::ARRAY_MATMODE);

	struct GhostInstance {
		const S3DModel* model;
		uint32_t worldTransformOffset;
		uint16_t paletteIndex; // color the ghost was last seen under (see LiveGhostBuilding / GhostSolidObject)
	};
	// bind the texture once per group, accumulate, then one Submit (=one multidraw) per texture type.
	// buckets are reused across frames (see clearBuckets) so a screen full of ghosts does not realloc
	// its per-texture vectors every frame; empty buckets (a texture no longer on screen) are skipped.
	const auto flushGhosts = [&](const std::map<int, std::vector<GhostInstance>>& byTex) {
		for (const auto& [texType, instances] : byTex) {
			if (instances.empty())
				continue;
			CModelDrawerHelper::BindModelTypeTexture(modelType, texType);
			for (const auto& gi : instances)
				smv.AddStaticInstance(gi.model, gi.worldTransformOffset, gi.paletteIndex);
			smv.Submit(GL_TRIANGLES, false);
		}
	};
	// clear the mapped vectors (keeping their capacity) instead of clearing the map (which would free them)
	const auto clearBuckets = [](std::map<int, std::vector<GhostInstance>>& byTex) {
		for (auto& [texType, instances] : byTex)
			instances.clear();
	};

	// deadGhostedBuildings (single color state)
	{
		const auto& deadGhostBuildings = modelDrawerData->GetDeadGhostBuildings(gu->myAllyTeam, modelType);

		static std::map<int, std::vector<GhostInstance>> byTex;
		clearBuckets(byTex);
		bool any = false;
		for (const auto* dgb : deadGhostBuildings) {
			const S3DModel* model = dgb->GetModel();
			if (!camera->InView(dgb->pos, model->GetDrawRadius()))
				continue;
			if (!dgb->worldTransformAlloc.Valid())
				continue;

			byTex[model->textureType].push_back({ model, static_cast<uint32_t>(dgb->worldTransformAlloc.GetOffset()), dgb->paletteIndex });
			any = true;
		}

		if (any) {
			modelDrawerState->SetColorMultiplier(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);
			modelDrawerState->SetTeamColor(0, IModelDrawerState::alphaValues.y); //teamID is per-instance
			flushGhosts(byTex);
		}
	}

	// liveGhostedBuildings (two color states: normal and CONTRADAR)
	{
		const auto& liveGhostedBuildings = modelDrawerData->GetLiveGhostBuildings(gu->myAllyTeam, modelType);

		static std::map<int, std::vector<GhostInstance>> byTexNormal;
		static std::map<int, std::vector<GhostInstance>> byTexContradar;
		clearBuckets(byTexNormal);
		clearBuckets(byTexContradar);
		bool anyNormal = false;
		bool anyContradar = false;

		for (const auto& lgb : liveGhostedBuildings) {
			const CUnit* u = lgb.unit;
			if (!camera->InView(u->pos, u->model->GetDrawRadius()))
				continue;

			// check for decoy models
			const UnitDef* decoyDef = u->unitDef->decoyDef;
			const S3DModel* model = (decoyDef == nullptr) ? u->model : decoyDef->LoadModel();

			// FIXME: needs a second pass
			if (model->type != modelType)
				continue;

			const size_t xfOffset = modelDrawerData->GetLiveGhostTransform(u);
			if (xfOffset == TransformsMemStorage::INVALID_INDEX)
				continue;

			const unsigned short losStatus = u->losStatus[gu->myAllyTeam];
			const bool contradar = (losStatus & LOS_CONTRADAR);
			// bucket with the palette the unit was last seen under, not the live unit's current one
			(contradar ? byTexContradar : byTexNormal)[model->textureType]
				.push_back({ model, static_cast<uint32_t>(xfOffset), lgb.paletteIndex });
			(contradar ? anyContradar : anyNormal) = true;
		}

		if (anyNormal) {
			modelDrawerState->SetColorMultiplier(0.6f, 0.6f, 0.6f, IModelDrawerState::alphaValues.y);
			modelDrawerState->SetTeamColor(0, IModelDrawerState::alphaValues.y);
			flushGhosts(byTexNormal);
		}
		if (anyContradar) {
			modelDrawerState->SetColorMultiplier(0.9f, 0.9f, 0.9f, IModelDrawerState::alphaValues.z);
			modelDrawerState->SetTeamColor(0, IModelDrawerState::alphaValues.z);
			flushGhosts(byTexContradar);
		}
	}

	modelDrawerState->SetColorMultiplier(IModelDrawerState::alphaValues.x);
	modelDrawerState->SetMatrixMode(oldMM); //reset is needed because other modelType's might be rendered afterwards
	smv.Unbind();
}

void CUnitDrawerGL4::DrawAlphaObjectsAux(int modelType) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpAlphaUnits = modelDrawerData->GetTempAlphaDrawUnits(modelType);
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	const auto oldMM = modelDrawerState->SetMatrixMode(ShaderMatrixModes::STATIC_MATMODE);

	// NOTE: not type-sorted
	for (const auto& unit : tmpAlphaUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawAlphaAIUnit(unit);
		DrawAlphaAIUnitBorder(unit);
	}

	modelDrawerState->SetMatrixMode(oldMM); //reset is needed because other modelType's might be rendered afterwards
	smv.Unbind();
}

void CUnitDrawerGL4::DrawAlphaAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	static CMatrix44f staticWorldMat;

	staticWorldMat.LoadIdentity();
	staticWorldMat.Translate(unit.pos);

	staticWorldMat.RotateY(unit.rotation);

	auto& smv = S3DModelVAO::GetInstance(); //bound already

	const UnitDef* def = unit.GetUnitDef();
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	CModelDrawerHelper::BindModelTypeTexture(mdl->type, mdl->textureType);

	SetTeamColor(unit.team, IModelDrawerState::alphaValues.x);
	modelDrawerState->SetStaticModelMatrix(staticWorldMat);

	smv.SubmitImmediately(mdl, static_cast<uint16_t>(unit.team));
}

void CUnitDrawerGL4::DrawOpaqueObjectsAux(int modelType) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const std::vector<CUnitDrawerData::TempDrawUnit>& tmpOpaqueUnits = modelDrawerData->GetTempOpaqueDrawUnits(modelType);
	auto& smv = S3DModelVAO::GetInstance();
	smv.Bind();

	const auto oldMM = modelDrawerState->SetMatrixMode(ShaderMatrixModes::STATIC_MATMODE);

	// NOTE: not type-sorted
	for (const auto& unit : tmpOpaqueUnits) {
		if (!camera->InView(unit.pos, 100.0f))
			continue;

		DrawOpaqueAIUnit(unit);
	}

	modelDrawerState->SetMatrixMode(oldMM); //reset is needed because other modelType's might be rendered afterwards

	smv.Unbind();
}

void CUnitDrawerGL4::DrawOpaqueAIUnit(const CUnitDrawerData::TempDrawUnit& unit) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	static CMatrix44f staticWorldMat;

	staticWorldMat.LoadIdentity();
	staticWorldMat.Translate(unit.pos);

	staticWorldMat.RotateY(unit.rotation);

	auto& smv = S3DModelVAO::GetInstance(); //bound already

	const UnitDef* def = unit.GetUnitDef();
	const S3DModel* mdl = def->model;

	assert(mdl != nullptr);

	CModelDrawerHelper::BindModelTypeTexture(mdl->type, mdl->textureType);

	SetTeamColor(unit.team);
	modelDrawerState->SetStaticModelMatrix(staticWorldMat);

	smv.SubmitImmediately(mdl, static_cast<uint16_t>(unit.team));
}

void CUnitDrawerGL4::DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto& smv = S3DModelVAO::GetInstance();

	const float3 stageBounds = { 0.0f, unit->model->CalcDrawHeight(), unit->buildProgress };

	const float4 upperPlanes[] = {
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f       )},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const float4 lowerPlanes[] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 3.0f  - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                                           0.0f },
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	Shader::IProgramObject* po = shadowHandler.GetShadowGenProg(CShadowHandler::SHADOWGEN_PROGRAM_MODEL_GL4);
	assert(po);
	assert(po->IsBound());

	glPushAttrib(GL_POLYGON_BIT);

	glEnable(GL_CLIP_DISTANCE0);
	glEnable(GL_CLIP_DISTANCE1);

	const auto SetClipPlane = [po](uint8_t idx, const float4& cp) {
		switch (idx)
		{
		case 0: //upper construction clip plane
			po->SetUniform("clipPlane0", cp.x, cp.y, cp.z, cp.w);
			break;
		case 1: //lower construction clip plane
			po->SetUniform("clipPlane1", cp.x, cp.y, cp.z, cp.w);
			break;
		default:
			assert(false);
			break;
		}
	};

	{
		// wireframe, unconditional
		SetClipPlane(0, upperPlanes[BUILDSTAGE_WIRE]);
		SetClipPlane(1, lowerPlanes[BUILDSTAGE_WIRE]);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		smv.SubmitImmediately(unit, GL_TRIANGLES);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (stageBounds.z > 1.0f / 3.0f) {
		// flat-colored, conditional
		SetClipPlane(0, upperPlanes[BUILDSTAGE_FLAT]);
		SetClipPlane(1, lowerPlanes[BUILDSTAGE_FLAT]);

		smv.SubmitImmediately(unit, GL_TRIANGLES);
	}

	SetClipPlane(0, float4{ 0.0f, 0.0f, 0.0f, 1.0f }); //default
	SetClipPlane(1, float4{ 0.0f, 0.0f, 0.0f, 1.0f }); //default;

	glDisable(GL_CLIP_DISTANCE1);
	glDisable(GL_CLIP_DISTANCE0);

	if (stageBounds.z > 2.0f / 3.0f) {
		// fully-shaded, conditional
		smv.SubmitImmediately(unit, GL_TRIANGLES);
	}

	glPopAttrib();
}

void CUnitDrawerGL4::DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto& smv = S3DModelVAO::GetInstance();

	const    CTeam* team = teamHandler.Team(unit->team);
	const   SColor  color = team->color;

	const float wireColorMult = std::fabs(128.0f - ((gs->frameNum * 4) & 255)) / 255.0f + 0.5f;
	const float flatColorMult = 1.5f - wireColorMult;

	const float3 frameColors[2] = { unit->unitDef->nanoColor, {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f} };
	const float3 stageColors[2] = { frameColors[globalRendering->teamNanospray], frameColors[globalRendering->teamNanospray] };


	const float3 stageBounds = { 0.0f, unit->model->CalcDrawHeight(), unit->buildProgress };

	// draw-height defaults to maxs.y - mins.y, but can be overridden for non-3DO models
	// the default value derives from the model vertices and makes more sense to use here
	//
	// Both clip planes move up. Clip plane 0 is the upper bound of the model,
	// clip plane 1 is the lower bound. In other words, clip plane 0 makes the
	// wireframe/flat color/texture appear, and clip plane 1 then erases the
	// wireframe/flat color later on.
	const float4 upperPlanes[] = {
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f       )},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 1.0f)},
		{0.0f, -1.0f, 0.0f,  stageBounds.x + stageBounds.y * (stageBounds.z * 3.0f - 2.0f)},
		{0.0f,  0.0f, 0.0f,                                                          0.0f },
	};
	const float4 lowerPlanes[] = {
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z * 10.0f - 9.0f)},
		{0.0f,  1.0f, 0.0f, -stageBounds.x - stageBounds.y * (stageBounds.z *  3.0f - 2.0f)},
		{0.0f,  1.0f, 0.0f,                                  (                        0.0f)},
		{0.0f,  0.0f, 0.0f,                                                           0.0f },
	};

	glPushAttrib(GL_POLYGON_BIT);

	glEnable(GL_CLIP_DISTANCE0);
	glEnable(GL_CLIP_DISTANCE1);

	{
		// wireframe, unconditional
		SetNanoColor(float4(stageColors[0] * wireColorMult, 1.0f));
		modelDrawerState->SetClipPlane(0, upperPlanes[BUILDSTAGE_WIRE]);
		modelDrawerState->SetClipPlane(1, lowerPlanes[BUILDSTAGE_WIRE]);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		smv.SubmitImmediately(unit, GL_TRIANGLES);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (stageBounds.z > 1.0f / 3.0f) {
		// flat-colored, conditional
		SetNanoColor(float4(stageColors[1] * flatColorMult, 1.0f));
		modelDrawerState->SetClipPlane(0, upperPlanes[BUILDSTAGE_FLAT]);
		modelDrawerState->SetClipPlane(1, lowerPlanes[BUILDSTAGE_FLAT]);

		smv.SubmitImmediately(unit, GL_TRIANGLES);
	}

	modelDrawerState->SetClipPlane(1); //default;
	glDisable(GL_CLIP_DISTANCE1);

	if (stageBounds.z > 2.0f / 3.0f) {
		// fully-shaded, conditional
		glPolygonOffset(1.0f, 1.0f);
		glEnable(GL_POLYGON_OFFSET_FILL);
		SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f));
		modelDrawerState->SetClipPlane(0, upperPlanes[BUILDSTAGE_FLAT]);

		smv.SubmitImmediately(unit, GL_TRIANGLES);

		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	SetNanoColor(float4(1.0f, 1.0f, 1.0f, 0.0f)); // turn off in any case
	modelDrawerState->SetClipPlane(0); //default
	glDisable(GL_CLIP_DISTANCE0);

	glPopAttrib();
}
