/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <cctype>
#include <cmath>

#include "GroundDecal.h"
#include "GroundDecalHandler.h"
#include "Game/Camera.h"
#include "Game/GameHelper.h"
#include "Game/GameSetup.h"
#include "Game/GlobalUnsynced.h"
#include "Lua/LuaParser.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/TexBind.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/GL/glHelpers.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Textures/Bitmap.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Features/FeatureDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Projectiles/ExplosionListener.h"
#include "Sim/Weapons/WeaponDef.h"
#include "Sim/MoveTypes/MoveType.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/creg/STL_Variant.h"
#include "System/creg/STL_Tuple.h"
#include "fmt/format.h"

#include "System/Misc/TracyDefs.h"

CONFIG(int, GroundScarAlphaFade).deprecated(true);
CONFIG(bool, HighQualityDecals).defaultValue(false).description("Forces MSAA processing of decals. Improves decals quality, but may ruin the performance.");

CR_BIND(CGroundDecalHandlerData::UnitMinMaxHeight, )
CR_REG_METADATA_SUB(CGroundDecalHandlerData, UnitMinMaxHeight,
(
	CR_MEMBER(min),
	CR_MEMBER(max)
))

CR_BIND_DERIVED(CGroundDecalHandlerData, IGroundDecalDrawer, )

CR_REG_METADATA(CGroundDecalHandlerData, (
	CR_MEMBER_UN(maxUniqueScars),
	CR_MEMBER_UN(atlasTex),
	CR_MEMBER_UN(decalShader),

	CR_MEMBER(decalOwners),
	CR_MEMBER(unitMinMaxHeights),
	CR_MEMBER(idToPos),
	CR_MEMBER(idToCmInfo),
	CR_MEMBER_UN(texFileNames),

	CR_MEMBER(decalsUpdateList),

	CR_MEMBER(nextId),
	CR_MEMBER(freeIds),

	CR_MEMBER_UN(instVBO),
	CR_MEMBER_UN(vao),
	CR_MEMBER_UN(smfDrawer),

	CR_MEMBER_UN(highQuality),
	CR_MEMBER_UN(sdbc),

	CR_POSTLOAD(PostLoad)
))

CGroundDecalHandlerData::CGroundDecalHandlerData()
	: IGroundDecalDrawer()
	, maxUniqueScars{ 0 }
	, atlasTex{ nullptr }
	, decalShader{ nullptr }
	, decalsUpdateList{ }
	, smfDrawer{ nullptr }
	, highQuality{ configHandler->GetBool("HighQualityDecals") && (globalRendering->msaaLevel > 0) }
	, sdbc{ highQuality }
{
}

void CGroundDecalHandlerData::PostLoad()
{
	decalsUpdateList.SetNeedUpdateAll();
}

CGroundDecalHandler::CGroundDecalHandler()
	: CEventClient("[CGroundDecalHandler]", 314159, false)
	, CGroundDecalHandlerData()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!GetDrawDecals())
		return;

	eventHandler.AddClient(this);
	CExplosionCreator::AddExplosionListener(this);

	configHandler->NotifyOnChange(this, { "HighQualityDecals", "UnitGhostIconsDimming" });

	smfDrawer = dynamic_cast<CSMFGroundDrawer*>(readMap->GetGroundDrawer());

	GenerateAtlasTexture();
	ReloadDecalShaders();

	instVBO = VBO(GL_ARRAY_BUFFER, false, false);

	decals.reserve(1 << 16);
	decalsUpdateList.Reserve(decals.capacity());

	nextId = 0;
}

CGroundDecalHandler::~CGroundDecalHandler()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CExplosionCreator::RemoveExplosionListener(this);
	eventHandler.RemoveClient(this);
	configHandler->RemoveObserver(this);

	shaderHandler->ReleaseProgramObjects("[GroundDecalHandler]");
	decalShader = nullptr;
	atlasTex = nullptr;
}

namespace Impl {
	enum class LoadResult {
		SUCCESS = 0,
		FILE_NOT_FOUND = 1,
		BITMAP_ERROR = 2
	};

	std::tuple<LoadResult, CBitmap, std::string> LoadTexture(const std::string& name, bool convertDecalBitmap, const std::string& errMsg)
	{
		RECOIL_DETAILED_TRACY_ZONE;
		std::string fileName = StringToLower(name);

		if (FileSystem::GetExtensionLowerCase(fileName).empty())
			fileName += ".bmp";

		std::string fullName = fileName;

		if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL))
			fullName = std::string("bitmaps/") + fileName;

		if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL))
			fullName = std::string("unittextures/") + fileName;

		if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL)) {
			return std::make_tuple(LoadResult::FILE_NOT_FOUND, CBitmap(), fileName);
		}

		CBitmap bm;
		if (!bm.Load(fullName))
			return std::make_tuple(LoadResult::BITMAP_ERROR, CBitmap(), fullName);

		if (convertDecalBitmap && FileSystem::GetExtensionLowerCase(fullName) == "bmp") {
			// bitmaps don't have an alpha channel
			// so use: red := brightness & green := alpha
			auto* rmem = bm.GetRawMem();

			for (int y = 0; y < bm.ysize; ++y) {
				for (int x = 0; x < bm.xsize; ++x) {
					const int index = ((y * bm.xsize) + x) * 4;

					const auto brightness = rmem[index + 0];
					const auto alpha = rmem[index + 1];

					rmem[index + 0] = (brightness * 90) / 255;
					rmem[index + 1] = (brightness * 60) / 255;
					rmem[index + 2] = (brightness * 30) / 255;
					rmem[index + 3] = alpha;
				}
			}
		}
		// non BMP scar textures doesn't follow the above historic convention, so keep them as is

		return std::make_tuple(LoadResult::SUCCESS, bm, fullName);
	}
}

static inline std::string GetExtraTextureName(const std::string& mainTex) {
	RECOIL_DETAILED_TRACY_ZONE;
	auto dotPos = mainTex.find_last_of(".");
	return mainTex.substr(0, dotPos) + "_normal" + (dotPos == string::npos ? "" : mainTex.substr(dotPos));
}

void CGroundDecalHandler::AddTexToAtlas(const std::string& name, const std::string& filename, bool convertOldBMP, const std::string& errMsg, bool reportMissingFile) {
	RECOIL_DETAILED_TRACY_ZONE;

	const auto [res, bm, fn] = Impl::LoadTexture(filename, convertOldBMP, errMsg);
	if (res == Impl::LoadResult::SUCCESS) {
		if (atlasTex->AddTexFromBitmap(name, bm, filename)) {
			texFileNames.emplace(name, fn);
		}
	}
	else if (res == Impl::LoadResult::BITMAP_ERROR) {
		LOG_L(L_WARNING, "%s", fmt::format("{}. Failed loading decal bitmap: \"{}\"", errMsg, fn).c_str());
	}
	else if (reportMissingFile) {
		LOG_L(L_WARNING, "%s", fmt::format("{}. Decal file not found: \"{}\"", errMsg, fn).c_str());
	}
}

void CGroundDecalHandler::AddBuildingDecalTextures()
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto CreateFallBackTexture = [](const SColor& color) {
		CBitmap bm;
		bm.AllocDummy(color);
		bm = bm.CreateRescaled(32, 32);
		return bm.CreateTexture();
	};

	auto ProcessDefs = [this](const auto& defsVector, const std::string& defName) {
		for (const SolidObjectDef& soDef : defsVector) {
			const SolidObjectDecalDef& decalDef = soDef.decalDef;

			if (!decalDef.useGroundDecal)
				continue;

			if (decalDef.groundDecalTypeName.empty())
				continue;

			const std::string mainTex =                    (decalDef.groundDecalTypeName);
			const std::string normTex = GetExtraTextureName(decalDef.groundDecalTypeName);

			const std::string ERR_MSG = fmt::format("Error loading a ground decal texture from {}Defs, def.name = {}", defName, soDef.name);

			AddTexToAtlas(mainTex, mainTex, false, ERR_MSG,  true);
			AddTexToAtlas(normTex, normTex, false, ERR_MSG, false);
		}
	};

	ProcessDefs(featureDefHandler->GetFeatureDefsVec(), "Feature");
	ProcessDefs(unitDefHandler->GetUnitDefsVec(), "Unit");
}


void CGroundDecalHandler::AddTexturesFromTable()
{
	RECOIL_DETAILED_TRACY_ZONE;
	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		LOG_L(L_ERROR, "Failed to load resources: %s", resourcesParser.GetErrorLog().c_str());
	}

	const std::string ERR_MSG_SCAR1 = "Error loading a ground scar texture";

	const auto GraphicsTbl = resourcesParser.GetRoot().SubTable("graphics");
	const LuaTable scarsTable = GraphicsTbl.SubTable("scars");
	const int scarTblSize = scarsTable.GetLength();

	maxUniqueScars = 0;
	for (int i = 1; i <= scarTblSize; ++i) {
		const std::string mainTexFileName = scarsTable.GetString(i, "");

		if (mainTexFileName.find("_normal") != std::string::npos)
			continue;

		const std::string normTexFileName = mainTexFileName.empty() ? "" : GetExtraTextureName(mainTexFileName);
		const auto mainName = IntToString(i, "mainscar_%i");
		const auto normName = IntToString(i, "normscar_%i");

		AddTexToAtlas(mainName, mainTexFileName,  true, ERR_MSG_SCAR1,  true);
		AddTexToAtlas(normName, normTexFileName, false, ERR_MSG_SCAR1, false);

		// check if loaded for real
		maxUniqueScars += atlasTex->TextureExists(mainName);
	}

	// fill the gaps in case the loop above failed to load some of the scar textures
	if (maxUniqueScars != scarTblSize) {
		const std::string ERR_MSG_SCAR2 = "Error loading a replacement ground scar texture";

		const std::vector<std::string> scarMainTextures = CFileHandler::FindFiles("bitmaps/scars/", "scar?.*");
		const size_t scarsExtraNum = scarMainTextures.size();

		if (scarsExtraNum > 0) {
			for (int extraTexNum = 0, i = 1; scarTblSize - maxUniqueScars > 0 && i <= scarTblSize; ++i) {
				const auto mainName = IntToString(i, "mainscar_%i");
				if (atlasTex->GetAllocator()->contains(mainName))
					continue;

				const auto normName = IntToString(i, "normscar_%i");

				const std::string mainTexFileName = scarMainTextures[extraTexNum++ % scarsExtraNum];
				const std::string normTexFileName = GetExtraTextureName(mainTexFileName);

				AddTexToAtlas(mainName, mainTexFileName,  true, ERR_MSG_SCAR2,  true);
				AddTexToAtlas(normName, normTexFileName, false, ERR_MSG_SCAR2, false);

				maxUniqueScars += atlasTex->GetAllocator()->contains(mainName);
			}
		}
	}

	const std::string ERR_MSG_DECAL = "Error loading a user provided decal texture";

	const LuaTable decalsTable = GraphicsTbl.SubTable("decals");
	const int decalsTblSize = decalsTable.GetLength();
	for (int i = 1; i <= decalsTblSize; ++i) {
		const std::string mainTexFileName = decalsTable.GetString(i, "");

		if (mainTexFileName.find("_normal") != std::string::npos)
			continue;

		const std::string normTexFileName = mainTexFileName.empty() ? "" : GetExtraTextureName(mainTexFileName);
		const auto mainName = IntToString(i, "maindecal_%i");
		const auto normName = IntToString(i, "normdecal_%i");

		AddTexToAtlas(mainName, mainTexFileName,  true, ERR_MSG_DECAL,  true);
		AddTexToAtlas(normName, normTexFileName, false, ERR_MSG_DECAL, false);
	}
}

void CGroundDecalHandler::AddGroundTrackTextures()
{
	RECOIL_DETAILED_TRACY_ZONE;

	const std::string ERR_MSG = "Error loading a ground track/footprint decal texture";

	const auto fileNames = CFileHandler::FindFiles("bitmaps/tracks/", "");
	for (const auto& mainTexFileName : fileNames) {
		const auto mainName = FileSystem::GetBasename(StringToLower(mainTexFileName));
		const auto normName = mainName + "_norm";
		const std::string normTexFileName = GetExtraTextureName(mainTexFileName);

		AddTexToAtlas(mainName, mainTexFileName,  true, ERR_MSG,  true);
		AddTexToAtlas(normName, normTexFileName, false, ERR_MSG, false);
	}
}


void CGroundDecalHandler::AddFallbackTextures()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto minDim = std::max(atlasTex->GetMinDim(), 32);
	atlasTex->AddTex("%FB_MAIN%", minDim, minDim, SColor(255,   0,   0, 255), "%FB_MAIN%");
	atlasTex->AddTex("%FB_NORM%", minDim, minDim, SColor(128, 128, 255,   0), "%FB_NORM%");
}

uint32_t CGroundDecalHandler::GetNextId()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (freeIds.empty())
		return ++nextId;

	const auto newId = freeIds.back(); freeIds.pop_back();
	return newId;

	return 0;
}

void CGroundDecalHandler::BindVertexAttributes()
{
	for (const AttributeDef& ad : GroundDecal::attributeDefs) {
		glEnableVertexAttribArray(ad.index);
		glVertexAttribDivisor(ad.index, 1);

		if (ad.type == GL_FLOAT || ad.normalize)
			glVertexAttribPointer(ad.index, ad.count, ad.type, ad.normalize, ad.stride, ad.data);
		else //assume int types
			glVertexAttribIPointer(ad.index, ad.count, ad.type, ad.stride, ad.data);
	}
}

void CGroundDecalHandler::UnbindVertexAttributes()
{
	for (const AttributeDef& ad : GroundDecal::attributeDefs) {
		glDisableVertexAttribArray(ad.index);
		glVertexAttribDivisor(ad.index, 0);
	}
}

uint32_t CGroundDecalHandler::GetDepthBufferTextureTarget() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return highQuality ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
}

static constexpr auto DEFAULT_ALLOC_TYPE = CTextureAtlas::ATLAS_ALLOC_MP_LEGACY;
static constexpr auto DEFAULT_NUM_OF_TEXTURE_LEVELS = 4;
void CGroundDecalHandler::GenerateAtlasTexture() {
	atlasTex = std::make_unique<CTextureRenderAtlas>(DEFAULT_ALLOC_TYPE, 0, 0, DEFAULT_NUM_OF_TEXTURE_LEVELS, GL_RGBA8, "Decals");

	// often represented by compressed textures, cannot be added to the regular atlas
	AddBuildingDecalTextures();

	AddTexturesFromTable();
	AddGroundTrackTextures();
	AddFallbackTextures();

	if (!atlasTex->CalculateAtlas()) {
		LOG_L(L_ERROR, "Could not calculate %s texture atlas. Use fewer/smaller textures.", atlasTex->GetAtlasName().c_str());
	}
}

bool CGroundDecalHandler::ReloadDecalShaders() {
	if (shaderHandler->ReleaseProgramObjects("[GroundDecalHandler]"))
		decalShader = nullptr;

	decalShader = shaderHandler->CreateProgramObject("[GroundDecalHandler]", "DecalShaderGLSL");
	decalShader->LoadFromLua("shaders/GLSL/groundDecals.lua");

	decalShader->SetFlag("DEPTH_CLIP01", globalRendering->supportClipSpaceControl);
	decalShader->SetFlag("HAVE_SHADOWS", true);
	decalShader->SetFlag("HIGH_QUALITY", highQuality);
	decalShader->SetFlag("HAVE_INFOTEX", true);
	decalShader->SetFlag("SMF_WATER_ABSORPTION", true);
	decalShader->SetFlag("USE_TEXTURE_ARRAY", atlasTex->GetTexTarget() == GL_TEXTURE_2D_ARRAY);

	decalShader->BindAttribLocations<GroundDecal>();

	decalShader->Link();

	decalShader->Enable();
	decalShader->SetUniform("mapDims",
		static_cast<float>(mapDims.mapx * SQUARE_SIZE),
		static_cast<float>(mapDims.mapy * SQUARE_SIZE),
		1.0f / (mapDims.mapx * SQUARE_SIZE),
		1.0f / (mapDims.mapy * SQUARE_SIZE)
	);
	decalShader->SetUniform("mapDimsPO2",
		static_cast<float>(mapDims.pwr2mapx * SQUARE_SIZE),
		static_cast<float>(mapDims.pwr2mapy * SQUARE_SIZE),
		1.0f / (mapDims.pwr2mapx * SQUARE_SIZE),
		1.0f / (mapDims.pwr2mapy * SQUARE_SIZE)
	);

	decalShader->SetUniform("atlasTex"       , 0);
	decalShader->SetUniform("miniMapTex"     , 2);
	decalShader->SetUniform("heightTex"      , 3);
	decalShader->SetUniform("depthTex"       , 4);
	decalShader->SetUniform("groundNormalTex", 5);
	decalShader->SetUniform("shadowTex"      , 6);
	decalShader->SetUniform("shadowColorTex" , 7);
	decalShader->SetUniform("infoTex"        , 8);

	decalShader->SetUniform("waterMinColor"   , 0.0f, 0.0f, 0.0f);
	decalShader->SetUniform("waterBaseColor"  , 0.0f, 0.0f, 0.0f);
	decalShader->SetUniform("waterAbsorbColor", 0.0f, 0.0f, 0.0f);

	decalShader->SetUniform("curAdjustedFrame", std::max(gs->frameNum, 0) + globalRendering->timeOffset);
	decalShader->SetUniform("screenSizeInverse",
		1.0f / globalRendering->viewSizeX,
		1.0f / globalRendering->viewSizeY
	);
	const auto& identityMat = CMatrix44f::Identity();
	decalShader->SetUniformMatrix4x4("shadowMatrix", false, &identityMat.m[0]);

	decalShader->Disable();
	SunChanged();

	return decalShader->Validate();
}

void CGroundDecalHandler::BindTextures()
{
	RECOIL_DETAILED_TRACY_ZONE;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(atlasTex->GetTexTarget(), atlasTex->GetTexID());

	const CSMFReadMap* smfMap = smfDrawer->GetReadMap();

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetMiniMapTexture());

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetHeightMapTexture());

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GetDepthBufferTextureTarget(), depthBufferCopy->GetDepthBufferTexture(highQuality));

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetNormalsTexture());

	if (shadowHandler.ShadowsLoaded()) {
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE6, true);

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, shadowHandler.GetColorTextureID());
	}

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());

	glActiveTexture(GL_TEXTURE0);
}

void CGroundDecalHandler::UnbindTextures()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(atlasTex->GetTexTarget(), 0);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GetDepthBufferTextureTarget(), 0);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (smfDrawer->UseAdvShading() && shadowHandler.ShadowsLoaded()) {
		shadowHandler.ResetShadowTexSampler(GL_TEXTURE6, true);

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
}

/*
void CGroundDecalHandler::AddDecal(CUnit* unit, const float3& newPos)
{
	if (!GetDrawDecals())
		return;

	MoveSolidObject(unit, newPos);
}
*/


void CGroundDecalHandler::AddExplosion(AddExplosionInfo&& ei)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!GetDrawDecals())
		return;

	if (maxUniqueScars == 0)
		return;

	const float groundHeight = CGround::GetHeightReal(ei.pos.x, ei.pos.z, false);
	const float altitude = ei.pos.y - groundHeight;

	const WeaponDef::Visuals& vi = ei.wd ? ei.wd->visuals : WeaponDef::Visuals{};

	bool radiusOverride = (vi.scarDiameter >= 0.0f);
	ei.radius = mix(ei.radius, 0.5f * vi.scarDiameter, radiusOverride);

	// no decals for below-ground explosions
	// also no decals if they are too high in the air
	if (math::fabs(altitude) >= ei.radius)
		return;

	ei.pos.y -= altitude;
	ei.radius -= altitude;

	if (ei.radius < 5.0f)
		return;

	if (ei.damage < 5.0f)
		return;

	ei.damage = std::min(ei.damage, ei.radius * 30.0f);
	ei.damage *= (ei.radius / (ei.radius + altitude));
	if (!radiusOverride)
		ei.radius = std::min(ei.radius, ei.damage * 0.25f);

	if (ei.damage > 400.0f)
		ei.damage = 400.0f + std::sqrt(ei.damage - 400.0f);

	const float alpha = (vi.scarAlpha > 0.0f) ?
		vi.scarAlpha :
		std::clamp(2.0f * ei.damage / 255.0f, 0.8f, 1.0f);

	const float scarTTL = (vi.scarTtl > 0.0f) ?
		GAME_SPEED * vi.scarTtl :
		std::clamp(DECAL_LEVEL_MULT * 3.0f * ei.damage, 15.0f, DECAL_LEVEL_MULT * 1800.0f);

	const float glow = (vi.scarGlow > 0.0f) ?
		vi.scarGlow :
		std::clamp(2.0f * ei.damage / 255.0f, 0.0f, 1.0f);

	const float glowTTL = (vi.scarGlowTtl > 0.0f) ?
		GAME_SPEED * vi.scarGlowTtl :
		60.0f;

	const float alphaDecay = 1.0f / scarTTL;
	const float glowDecay = 1.0f / glowTTL;

	const float2 posTL = { ei.pos.x - ei.radius, ei.pos.z - ei.radius };
	const float2 posTR = { ei.pos.x + ei.radius, ei.pos.z - ei.radius };
	const float2 posBR = { ei.pos.x + ei.radius, ei.pos.z + ei.radius };
	const float2 posBL = { ei.pos.x - ei.radius, ei.pos.z + ei.radius };

	static std::vector<int> validScarIndices;
	validScarIndices.clear();
	for (auto scarIdx : vi.scarIdcs) {
		if (scarIdx < 1 || scarIdx > maxUniqueScars) // these are raw from Lua, so remember the +1 compared to C++
			continue;

		validScarIndices.emplace_back(scarIdx);
	}

	int scarIdx;
	if (validScarIndices.empty())
		scarIdx = 1 + guRNG.NextInt(maxUniqueScars); //not inclusive
	else
		scarIdx = validScarIndices[guRNG.NextInt(validScarIndices.size())]; //not inclusive

	const auto mainName = IntToString(scarIdx, "mainscar_%i");
	const auto normName = IntToString(scarIdx, "normscar_%i");

	const auto createFrame = static_cast<float>(std::max(gs->frameNum, 0));
	const auto height = argmax(ei.radius, ei.maxHeightDiff);

	std::array<SColor, 2> glowColorMap = { SColor{0.0f, 0.0f, 0.0f, 0.0f}, SColor{0.0f, 0.0f, 0.0f, 0.0f} };
	float cmAlphaMult = 1.0f;
	if (vi.scarGlowColorMap && !vi.scarGlowColorMap->Empty()) {
		auto idcs = vi.scarGlowColorMap->GetIndices(0.0f);
		glowColorMap[0] = vi.scarGlowColorMap->GetColor(idcs.first );
		glowColorMap[1] = vi.scarGlowColorMap->GetColor(idcs.second);
		cmAlphaMult = static_cast<float>(vi.scarGlowColorMap->GetMapSize());
	}

	const auto& decal = decals.emplace_back(GroundDecal{
		.refHeight = groundHeight,
		.minHeight = -ei.maxHeightDiff,
		.maxHeight =  ei.maxHeightDiff,
		.forceHeightMode = 1.0f,
		.posTL = posTL,
		.posTR = posTR,
		.posBR = posBR,
		.posBL = posBL,
		.texMainOffsets = static_cast<float4>(atlasTex->GetTexture(mainName, "%FB_MAIN%")),
		.texNormOffsets = static_cast<float4>(atlasTex->GetTexture(normName, "%FB_NORM%")),
		.alpha = alpha,
		.alphaFalloff = alphaDecay,
		.glow = glow,
		.glowFalloff = glowDecay,
		.rot = guRNG.NextFloat() * math::TWOPI,
		.height = height,
		.dotElimExp = vi.scarDotElimination,
		.cmAlphaMult = cmAlphaMult,
		.createFrameMin = createFrame,
		.createFrameMax = createFrame,
		.uvWrapDistance = 0.0f,
		.uvTraveledDistance = 0.0f,
		.forcedNormal = ei.projDir,
		.visMult = 1.0f,
		.info = GroundDecal::TypeID{ .type = static_cast<uint8_t>(GroundDecal::Type::DECAL_EXPLOSION), .id = GetNextId() },
		.tintColor = SColor{vi.scarColorTint},
		.glowColorMap = std::move(glowColorMap)
	});

	if (vi.scarGlowColorMap && !vi.scarGlowColorMap->Empty()) {
		auto idcs = vi.scarGlowColorMap->GetIndices(0.0f);
		idToCmInfo[decal.info.id] = std::make_tuple(vi.scarGlowColorMap, idcs);
	}

	idToPos[decal.info.id] = decals.size() - 1;
	decalsUpdateList.EmplaceBackUpdate();
}

void CGroundDecalHandler::ReloadTextures()
{
	RECOIL_DETAILED_TRACY_ZONE;

	spring::unordered_map<float4, std::string, float4Hash> subTexToNameMain;
	spring::unordered_map<float4, std::string, float4Hash> subTexToNameNorm;

	for (const auto& [name, ae] : atlasTex->GetAllocator()->GetEntries()) {
		subTexToNameMain.emplace(static_cast<float4>(atlasTex->GetTexture(name)), name);
	}

	// can't use {atlas}->ReloadTextures() here as all textures come from memory
	atlasTex = nullptr;
	GenerateAtlasTexture();

	// update scar subtexture names to match possibly a new number of maxUniqueScars
	for (auto& [at, name] : subTexToNameMain) {
		static constexpr std::string_view nameToFind = "mainscar_";
		auto it = name.find(nameToFind);
		if (it == std::string::npos)
			continue;

		int oldScarID = StringToInt(name.substr(nameToFind.length()));
		name = IntToString(1 + (oldScarID - 1) % maxUniqueScars, "mainscar_%i");
	}
	for (auto& [at, name] : subTexToNameNorm) {
		static constexpr std::string_view nameToFind = "normscar_";
		auto it = name.find(nameToFind);
		if (it == std::string::npos)
			continue;

		int oldScarID = StringToInt(name.substr(nameToFind.length()));
		name = IntToString(1 + (oldScarID - 1) % maxUniqueScars, "normscar_%i");
	}

	for (auto& decal : decals) {
		if (auto it = subTexToNameMain.find(decal.texMainOffsets); it != subTexToNameMain.end()) {
			decal.texMainOffsets = static_cast<float4>(atlasTex->GetTexture(it->second, "%FB_MAIN%"));
		}
		if (auto it = subTexToNameNorm.find(decal.texNormOffsets); it != subTexToNameNorm.end()) {
			decal.texNormOffsets = static_cast<float4>(atlasTex->GetTexture(it->second, "%FB_NORM%"));
		}
	}
	decalsUpdateList.SetNeedUpdateAll();
}

void CGroundDecalHandler::DumpAtlasTextures(const std::string& fileExt)
{
	RECOIL_DETAILED_TRACY_ZONE;
	atlasTex->DumpTexture(fileExt);
}

void CGroundDecalHandler::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!GetDrawDecals())
		return;

	if (!smfDrawer->UseAdvShading())
		return;

	if (decals.empty())
		return;

	if (!atlasTex->CreateAtlasTexture())
		return;

	UpdateDecalsVisibility();

	if (instVBO.GetSize() < decals.size() * sizeof(GroundDecal)) {
		vao.Bind();

		instVBO.Bind();
		instVBO.New(decals.capacity() * sizeof(GroundDecal), GL_STREAM_DRAW);
		BindVertexAttributes();

		vao.Unbind();

		UnbindVertexAttributes();
		instVBO.Unbind();
		decalsUpdateList.SetNeedUpdateAll();
	}

	if (decalsUpdateList.NeedUpdate()) {
		instVBO.Bind();

		for (auto itPair = decalsUpdateList.GetNext(); itPair.has_value(); itPair = decalsUpdateList.GetNext(itPair)) {
			auto offSize = decalsUpdateList.GetOffsetAndSize(itPair.value());
			GLintptr byteOffset = offSize.first  * sizeof(GroundDecal);
			GLintptr byteSize   = offSize.second * sizeof(GroundDecal);
			instVBO.SetBufferSubData(byteOffset, byteSize, decals.data() + offSize.first/* in elements */);
		}

		instVBO.Unbind();
		decalsUpdateList.ResetNeedUpdateAll();
	}

	using namespace GL::State;

	auto state = GL::SubState(
		SampleShading(highQuality ? GL_TRUE : GL_FALSE),
		MinSampleShading(1.0f),
		Blending(GL_TRUE),
		BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA),
		DepthMask(GL_FALSE),
		DepthTest(GL_FALSE),
		Culling(GL_TRUE),
		CullFace(GL_BACK)
	);

	BindTextures();

	const bool visWater = smfDrawer->GetReadMap()->HasVisibleWater();

	if (decalShader->IsReloadRequested()) {
		ReloadDecalShaders();
		decalShader->SetReloadComplete();
	}

	decalShader->SetFlag("HAVE_SHADOWS", shadowHandler.ShadowsLoaded());
	decalShader->SetFlag("HAVE_INFOTEX", infoTextureHandler->IsEnabled());
	decalShader->SetFlag("SMF_WATER_ABSORPTION", visWater);
	decalShader->Enable();

	if (visWater) {
		decalShader->SetUniform("waterMinColor", waterRendering->minColor.x, waterRendering->minColor.y, waterRendering->minColor.z);
		decalShader->SetUniform("waterBaseColor", waterRendering->baseColor.x, waterRendering->baseColor.y, waterRendering->baseColor.z);
		decalShader->SetUniform("waterAbsorbColor", waterRendering->absorb.x, waterRendering->absorb.y, waterRendering->absorb.z);
	}

	decalShader->SetUniform("infoTexIntensityMul", float(infoTextureHandler->InMetalMode()) + 1.0f);
	decalShader->SetUniform("curAdjustedFrame", std::max(gs->frameNum, 0) + globalRendering->timeOffset);
	if (shadowHandler.ShadowsLoaded())
		decalShader->SetUniformMatrix4x4("shadowMatrix", false, shadowHandler.GetShadowMatrixRaw());

	vao.Bind();
	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, decals.size());
	vao.Unbind();

	decalShader->Disable();

	UnbindTextures();
}

std::array<uint32_t, 3> CGroundDecalHandler::GetTexSize() const
{
	if (!atlasTex)
		return std::array<uint32_t, 3>{0};

	const auto& flatSize = atlasTex->GetAtlasSize();
	const auto numPages = atlasTex->GetAllocator()->GetNumPages();
	return std::array<uint32_t, 3>{
		static_cast<uint32_t>(flatSize.x),
		static_cast<uint32_t>(flatSize.y),
		numPages
	};
}

uint32_t CGroundDecalHandler::GetTexID() const
{
	return atlasTex ? atlasTex->GetTexID() : 0;
}

uint32_t CGroundDecalHandler::GetTexTarget() const
{
	return atlasTex ? atlasTex->GetTexTarget() : 0;
}

void CGroundDecalHandler::AddSolidObject(const CSolidObject* object) { MoveSolidObject(object, object->pos); }
void CGroundDecalHandler::MoveSolidObject(const CSolidObject* object, const float3& pos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!GetDrawDecals())
		return;

	const SolidObjectDecalDef& decalDef = object->GetDef()->decalDef;

	if (!decalDef.useGroundDecal || decalDef.groundDecalTypeName.empty())
		return;

	int sizex = decalDef.groundDecalSizeX * SQUARE_SIZE;
	int sizey = decalDef.groundDecalSizeY * SQUARE_SIZE;

	const float2 midPoint = float2(static_cast<int>(pos.x / SQUARE_SIZE), static_cast<int>(pos.z / SQUARE_SIZE)) * SQUARE_SIZE;
	const float midPointHeight = CGround::GetHeightReal(midPoint.x, midPoint.y);

	auto posTL = midPoint + float2(-sizex, -sizey);
	auto posTR = midPoint + float2( sizex, -sizey);
	auto posBR = midPoint + float2( sizex,  sizey);
	auto posBL = midPoint + float2(-sizex,  sizey);

	const float height = argmax(
		math::fabs(midPointHeight - CGround::GetHeightReal(posTL.x, posTL.y)),
		math::fabs(midPointHeight - CGround::GetHeightReal(posTR.x, posTR.y)),
		math::fabs(midPointHeight - CGround::GetHeightReal(posBR.x, posBR.y)),
		math::fabs(midPointHeight - CGround::GetHeightReal(posBL.x, posBL.y))
	) + 25.0f;

	const auto createFrame = static_cast<float>(std::max(gs->frameNum, 0));

	if (const auto doIt = decalOwners.find(object); doIt != decalOwners.end()) {
		assert(doIt->second < decals.size());
		auto& decal = decals[doIt->second];
		decal.posTL = posTL;
		decal.posTR = posTR;
		decal.posBR = posBR;
		decal.posBL = posBL;
		decal.height = height;
		decalsUpdateList.SetUpdate(doIt->second);
		return;
	}

	const auto& decal = decals.emplace_back(GroundDecal{
		.refHeight = 0.0f,
		.minHeight = 0.0f,
		.maxHeight = 0.0f,
		.forceHeightMode = 0.0f,
		.posTL = posTL,
		.posTR = posTR,
		.posBR = posBR,
		.posBL = posBL,
		.texMainOffsets = static_cast<float4>(atlasTex->GetTexture(                   (decalDef.groundDecalTypeName), "%FB_MAIN%")),
		.texNormOffsets = static_cast<float4>(atlasTex->GetTexture(GetExtraTextureName(decalDef.groundDecalTypeName), "%FB_NORM%")),
		.alpha = 1.0f,
		.alphaFalloff = 0.0f,
		.glow = 0.0f,
		.glowFalloff = 0.0f,
		.rot = -object->buildFacing * math::HALFPI,
		.height = height,
		.dotElimExp = 0.0f,
		.cmAlphaMult = 1.0f,
		.createFrameMin = createFrame,
		.createFrameMax = createFrame,
		.uvWrapDistance = 0.0f,
		.uvTraveledDistance = 0.0f,
		.forcedNormal = float3{},
		.visMult = 1.0f,
		.info = GroundDecal::TypeID{.type = static_cast<uint8_t>(GroundDecal::Type::DECAL_PLATE), .id = GetNextId() },
		.tintColor = SColor{0.5f, 0.5f, 0.5f, 0.5f},
		.glowColorMap = { SColor{0.0f, 0.0f, 0.0f, 0.0f}, SColor{0.0f, 0.0f, 0.0f, 0.0f} }
	});

	decalsUpdateList.EmplaceBackUpdate();
	idToPos[decal.info.id] = decals.size() - 1;
	decalOwners[object] = decals.size() - 1;
}

void CGroundDecalHandler::RemoveSolidObject(const CSolidObject* object, const GhostSolidObject* gb)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(object);

	const auto doIt = decalOwners.find(object);
	if (doIt == decalOwners.end()) {
		// it's ok for an object to not have any decals
		return;
	}

	const auto pos = doIt->second;
	decalOwners.erase(doIt);

	if (gb) {
		// gb is the new owner
		decalOwners.emplace(gb, pos);
		return;
	}

	assert(pos < decals.size());
	auto& decayingDecal = decals[pos];

	// we only care about DECAL_PLATE decals below
	if (decayingDecal.info.type != static_cast<uint8_t>(GroundDecal::Type::DECAL_PLATE)) {
		return;
	}

	const auto createFrame = static_cast<float>(std::max(gs->frameNum, 0));

	decayingDecal.alphaFalloff = object->GetDef()->decalDef.groundDecalDecaySpeed / GAME_SPEED;
	decayingDecal.createFrameMin = createFrame;
	decayingDecal.createFrameMax = createFrame;

	decalsUpdateList.SetUpdate(pos);
}

/**
 * @brief immediately remove an object's ground decal, if any (without fade out)
 */
void CGroundDecalHandler::ForceRemoveSolidObject(const CSolidObject* object)
{
	RECOIL_DETAILED_TRACY_ZONE;
	RemoveSolidObject(object, nullptr);
}

void CGroundDecalHandler::GhostDestroyed(const GhostSolidObject* gb) {
	RECOIL_DETAILED_TRACY_ZONE;
	const auto doIt = decalOwners.find(gb);
	if (doIt == decalOwners.end())
		return;

	assert(doIt->second < decals.size());
	auto& decal = decals[doIt->second];
	// just in case
	if (decal.info.type != static_cast<uint8_t>(GroundDecal::Type::DECAL_PLATE))
		return;

	decal.alpha = 0.0f;
	decalsUpdateList.SetUpdate(doIt->second);
	decalOwners.erase(doIt);
}

uint32_t CGroundDecalHandler::CreateLuaDecal()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto createFrame = static_cast<float>(std::max(gs->frameNum, 0));

	const auto& decal = decals.emplace_back(GroundDecal{
		.refHeight = 0.0f,
		.minHeight = 0.0f,
		.maxHeight = 0.0f,
		.forceHeightMode = 0.0f,
		.posTL = float2{},
		.posTR = float2{},
		.posBR = float2{},
		.posBL = float2{},
		.texMainOffsets = static_cast<float4>(atlasTex->GetTexture("%FB_MAIN%")),
		.texNormOffsets = static_cast<float4>(atlasTex->GetTexture("%FB_NORM%")),
		.alpha = 1.0f,
		.alphaFalloff = 0.0f,
		.glow = 0.0f,
		.glowFalloff = 0.0f,
		.rot = 0.0f,
		.height = 0.0f,
		.dotElimExp = 0.0f,
		.cmAlphaMult = 1.0f,
		.createFrameMin = createFrame,
		.createFrameMax = createFrame,
		.uvWrapDistance = 0.0f,
		.uvTraveledDistance = 0.0f,
		.forcedNormal = float3{},
		.visMult = 1.0f,
		.info = GroundDecal::TypeID{.type = static_cast<uint8_t>(GroundDecal::Type::DECAL_LUA), .id = GetNextId() },
		.tintColor = SColor{0.5f, 0.5f, 0.5f, 0.5f},
		.glowColorMap = { SColor{0.0f, 0.0f, 0.0f, 0.0f}, SColor{0.0f, 0.0f, 0.0f, 0.0f} }
	});
	decalsUpdateList.EmplaceBackUpdate();
	idToPos[decal.info.id] = decals.size() - 1;

	return decal.info.id;
}

bool CGroundDecalHandler::DeleteLuaDecal(uint32_t id)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return false;

	assert(it->second < decals.size());
	auto& decal = decals[it->second];
	if (!decal.IsValid())
		return false;

	if (decal.info.type != static_cast<uint8_t>(GroundDecal::Type::DECAL_LUA))
		return false;

	decal.MarkInvalid();
	decalsUpdateList.SetUpdate(it->second);

	return true;
}

GroundDecal* CGroundDecalHandler::GetDecalById(uint32_t id)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return nullptr;

	assert(it->second < decals.size());
	auto& decal = decals[it->second];
	if (!decal.IsValid())
		return nullptr;

	decalsUpdateList.SetUpdate(it->second);
	return &decal;
}

const GroundDecal* CGroundDecalHandler::GetDecalById(uint32_t id) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return nullptr;

	assert(it->second < decals.size());
	const auto& decal = decals[it->second];
	if (!decal.IsValid())
		return nullptr;

	return &decal;
}

bool CGroundDecalHandler::SetDecalTexture(uint32_t id, const std::string& texName, bool mainTex)
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return false;

	assert(it->second < decals.size());
	auto& decal = decals[it->second];
	if (!decal.IsValid())
		return false;

	auto& offset = mainTex ? decal.texMainOffsets : decal.texNormOffsets;

	const auto newOffset = atlasTex->GetTexture(texName);
	if (newOffset == AtlasedTexture::DefaultAtlasTexture)
		return false;

	const auto newOffsetf4 = static_cast<float4>(newOffset);

	// micro optimization
	if (newOffsetf4 == offset)
		return true;

	offset = newOffsetf4;
	decalsUpdateList.SetUpdate(it->second);
	return true;
}

std::string CGroundDecalHandler::GetDecalTexture(uint32_t id, bool mainTex) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return "";

	assert(it->second < decals.size());
	const auto& decal = decals[it->second];
	if (!decal.IsValid())
		return "";

	const auto& offset = mainTex ? decal.texMainOffsets : decal.texNormOffsets;

	const auto& nameToUST = atlasTex->GetNameToUniqueSubTexMap();
	for (auto& [name, _] : nameToUST) {
		const auto at = static_cast<float4>(atlasTex->GetTexture(name));
		if (at == offset)
			return name;
	}

	return "";
}

const std::vector<std::string> CGroundDecalHandler::GetDecalTextures(const std::optional<bool>& mainTex) const
{
	auto IsNormalTexture = [](const std::string& texName) {
		if (texName == "%FB_NORM%")
			return true;

		if (texName.find("_normal") != std::string::npos)
			return true;

		return false;
	};

	std::vector<std::string> ret;

	const auto& nameToUST = atlasTex->GetNameToUniqueSubTexMap();
	for (const auto& [name, ust] : nameToUST) {
		if (!mainTex.has_value()) {
			ret.emplace_back(name);
			continue;
		}

		const auto isNorm = IsNormalTexture(name);

		if ((mainTex.value() && !isNorm) || (!mainTex.value() && isNorm))
			ret.emplace_back(name);
	}

	std::sort(ret.begin(), ret.end());
	return ret;
}

const std::vector<std::string> CGroundDecalHandler::GetDecalTextureFileNames(const std::vector<std::string>& texList) const
{
	std::vector<std::string> ret;
	ret.reserve(texList.size());

	for (const auto& tex : texList) {
		if (auto it = texFileNames.find(tex); it != texFileNames.end()) {
			ret.emplace_back(it->second);
		}
		else {
			ret.emplace_back("");
		}
	}

	return ret;
}

const CSolidObject* CGroundDecalHandler::GetDecalSolidObjectOwner(uint32_t id) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (const auto& [owner, pos] : decalOwners) {
		if (!std::holds_alternative<const CSolidObject*>(owner))
			continue;

		assert(pos < decals.size());
		const auto& decal = decals[pos];

		if (!decal.IsValid())
			continue;

		if (id != decals[pos].info.id)
			continue;

		return std::get<const CSolidObject*>(owner);
	}

	return nullptr;
}

void CGroundDecalHandler::SetUnitLeaveTracks(CUnit* unit, bool leaveTracks)
{
	//ZoneScoped;
	unit->leaveTracks = leaveTracks;
	if (!leaveTracks) {
		if (auto it = decalOwners.find(unit); it != decalOwners.end()) {
			auto& mm = unitMinMaxHeights[unit->id];

			decalOwners.erase(it); // restart with new decal next time
			mm = {};
		}
	}
	else {
		AddTrack(unit, unit->pos, false);
	}
}

static inline bool CanReceiveTracks(const float3& pos)
{
	//ZoneScoped;
	// calculate typemap-index
	const int tmz = pos.z / (SQUARE_SIZE * 2);
	const int tmx = pos.x / (SQUARE_SIZE * 2);
	const int tmi = std::clamp(tmz * mapDims.hmapx + tmx, 0, mapDims.hmapx * mapDims.hmapy - 1);

	const uint8_t* typeMap = readMap->GetTypeMapSynced();
	const uint8_t  typeNum = typeMap[tmi];

	return mapInfo->terrainTypes[typeNum].receiveTracks;
}

void CGroundDecalHandler::AddTrack(const CUnit* unit, const float3& newPos, bool forceEval)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!GetDrawDecals())
		return;

	if (!unit->leaveTracks)
		return;

	if (!gu->spectatingFullView && !unit->IsInLosForAllyTeam(gu->myAllyTeam))
		return;

	const UnitDef* unitDef = unit->unitDef;

	if (!unitDef->IsGroundUnit())
		return;

	const SolidObjectDecalDef& decalDef = unitDef->decalDef;

	const float trackLifeTime = DECAL_LEVEL_MULT * GAME_SPEED * decalDef.trackDecalStrength;
	if (trackLifeTime <= 0.0f)
		return;

	const float3 decalPos = newPos + unit->frontdir * decalDef.trackDecalOffset;

	auto& mm = unitMinMaxHeights[unit->id];

	if (!CanReceiveTracks(decalPos) || (unit->IsInWater() && !unit->IsOnGround())) {
		decalOwners.erase(unit); // restart with new decal next time
		mm = {};
		return;
	}

	const float2 decalPos2 = float2(decalPos.x, decalPos.z);
	const float2 wc = float2(
		unit->rightdir.x * decalDef.trackDecalWidth * 0.5f,
		unit->rightdir.z * decalDef.trackDecalWidth * 0.5f
	);

	const auto createFrameInt = static_cast<uint32_t>(std::max(gs->frameNum, 0));
	const auto createFrame = static_cast<float>(createFrameInt);
	const auto doIt = decalOwners.find(unit);
	if (doIt == decalOwners.end()) {
		// new decal

		// the decal texture name is stored as a basename
		const auto& mainName = FileSystem::GetBasename(StringToLower(decalDef.trackDecalTypeName));
		const auto  normName = GetExtraTextureName(mainName);

		const float alphaDecay = 1.0f / trackLifeTime;

		const auto& decal = decals.emplace_back(GroundDecal{
			.refHeight = 0.0f,
			.minHeight = 0.0f,
			.maxHeight = 0.0f,
			.forceHeightMode = 0.0f,
			.posTL = decalPos2 - wc,
			.posTR = decalPos2 - wc,
			.posBR = decalPos2 + wc,
			.posBL = decalPos2 + wc,
			.texMainOffsets = static_cast<float4>(atlasTex->GetTexture(mainName, "%FB_MAIN%")),
			.texNormOffsets = static_cast<float4>(atlasTex->GetTexture(normName, "%FB_NORM%")),
			.alpha = 1.0f,
			.alphaFalloff = alphaDecay,
			.glow = 0.0f,
			.glowFalloff = 0.0f,
			.rot = 0.0f,
			.height = 0.0f,
			.dotElimExp = 0.0f,
			.cmAlphaMult = 1.0f,
			.createFrameMin = createFrame,
			.createFrameMax = createFrame,
			.uvWrapDistance = decalDef.trackDecalWidth * decalDef.trackDecalStretch,
			.uvTraveledDistance = 0.0f,
			.forcedNormal = float3{unit->updir},
			.visMult = 1.0f,
			.info = GroundDecal::TypeID{.type = static_cast<uint8_t>(GroundDecal::Type::DECAL_TRACK), .id = GetNextId() },
			.tintColor = SColor{0.5f, 0.5f, 0.5f, 0.5f},
			.glowColorMap = { SColor{0.0f, 0.0f, 0.0f, 0.0f}, SColor{0.0f, 0.0f, 0.0f, 0.0f} }
		});

		mm = {};

		decalOwners[unit] = decals.size() - 1;
		idToPos[decal.info.id] = decals.size() - 1;
		decalsUpdateList.EmplaceBackUpdate();

		return;
	}

	float decalHeight = CGround::GetHeightReal(decalPos.x, decalPos.z, false);
	mm.min = std::min(mm.min, decalHeight);
	mm.max = std::max(mm.max, decalHeight);

	if (!forceEval && createFrameInt % TRACKS_UPDATE_RATE != 0)
		return;

	assert(doIt->second < decals.size());
	GroundDecal& oldDecal = decals[doIt->second];

	// just updated
	if (oldDecal.createFrameMax == createFrame)
		return;

	// check if the unit is standing still
	if (oldDecal.createFrameMax + TRACKS_UPDATE_RATE < createFrame) {
		decalOwners.erase(unit);
		mm = {};
		return;
	}

	const float2 posL = (oldDecal.posTL + oldDecal.posBL) * 0.5f;
	const float2 posR = (oldDecal.posTR + oldDecal.posBR) * 0.5f;

	const float2 dirO = (posR      - posL).SafeNormalize();
	const float2 dirN = (decalPos2 - posR).SafeNormalize();

	// dirN was ~zero
	if (dirN.Dot(dirN) < 0.25f)
		return;

	// the old decal had zero len (was a new track decal) or similar dir and the unit updir is same-ish as before
	if ((dirO.Dot(dirO) == 0.0f || dirO.Dot(dirN) >= 0.9999f) && oldDecal.forcedNormal.dot(unit->updir) >= 0.99f) {
		oldDecal.posTR = decalPos2 - wc;
		oldDecal.posBR = decalPos2 + wc;
		oldDecal.createFrameMax = createFrame;

		const float2 midPointDist = (oldDecal.posTL + oldDecal.posTR + oldDecal.posBR + oldDecal.posBL) * 0.25f;
		const float midPointHeight = CGround::GetHeightReal(midPointDist.x, midPointDist.y, false);
		oldDecal.height = argmax(mm.max - midPointHeight, midPointHeight - mm.min) + 25.0f;

		decalsUpdateList.SetUpdate(doIt->second);
		return;
	}

	// new decal, starting where the previous ended
	auto& newDecal = decals.emplace_back(GroundDecal{
		.refHeight = 0.0f,
		.minHeight = 0.0f,
		.maxHeight = 0.0f,
		.forceHeightMode = 0.0f,
		.posTL = oldDecal.posTR,
		.posTR = decalPos2 - wc,
		.posBR = decalPos2 + wc,
		.posBL = oldDecal.posBR,
		.texMainOffsets = oldDecal.texMainOffsets,
		.texNormOffsets = oldDecal.texNormOffsets,
		.alpha = oldDecal.alpha,
		.alphaFalloff = oldDecal.alphaFalloff,
		.glow = 0.0f,
		.glowFalloff = 0.0f,
		.rot = 0.0f,
		.height = oldDecal.height, //also set later
		.dotElimExp = oldDecal.dotElimExp,
		.cmAlphaMult = oldDecal.cmAlphaMult,
		.createFrameMin = oldDecal.createFrameMax,
		.createFrameMax = createFrame,
		.uvWrapDistance = decalDef.trackDecalWidth * decalDef.trackDecalStretch,
		.uvTraveledDistance = oldDecal.uvTraveledDistance + posL.Distance(posR)/*oldDecal.posTL.Distance(oldDecal.posTR)*/,
		.forcedNormal = float3{ unit->updir },
		.visMult = 1.0f,
		.info = GroundDecal::TypeID{.type = static_cast<uint8_t>(GroundDecal::Type::DECAL_TRACK), .id = GetNextId() },
		.tintColor = SColor{0.5f, 0.5f, 0.5f, 0.5f},
		.glowColorMap = { SColor{0.0f, 0.0f, 0.0f, 0.0f}, SColor{0.0f, 0.0f, 0.0f, 0.0f} }
	});

	const float2 midPointDist = (newDecal.posTL + newDecal.posTR + newDecal.posBR + newDecal.posBL) * 0.25f;
	const float midPointHeight = CGround::GetHeightReal(midPointDist.x, midPointDist.y, false);
	newDecal.height = argmax(newDecal.height, mm.max - midPointHeight, midPointHeight - mm.min, 25.0f);
	mm = {};

	// replace the old entry
	decalOwners[unit] = decals.size() - 1;

	idToPos[newDecal.info.id] = decals.size() - 1;
	decalsUpdateList.EmplaceBackUpdate();
}

void CGroundDecalHandler::CompactDecalsVector(int frameNum)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (frameNum % 300 != 0)
		return;

	if (decals.empty())
		return;

	// only bother with the following code, if number of items is big enough
	if (decals.size() < decals.capacity() >> 6)
		return;

	size_t numToDelete = 0;
	for (auto& decal : decals) {
		if (!decal.IsValid()) {
			numToDelete++;
			continue;
		}
		const auto targetExpirationFrame = static_cast<int>(decal.alpha / std::max(decal.alphaFalloff, 1e-6f));
		if (decal.info.type != static_cast<uint8_t>(GroundDecal::Type::DECAL_LUA) && frameNum - decal.createFrameMax > targetExpirationFrame) {
			decal.MarkInvalid();
			numToDelete++;
		}
	}

	if (numToDelete == 0)
		return;

	// resort if number of expired items > 25.0%
	static constexpr float RESORT_THRESHOLD = 1.0f / 4.0f;
	if (static_cast<float>(decals.size()) / static_cast<float>(numToDelete) <= RESORT_THRESHOLD)
		return;

#if 0
	LOG("DH:CompactDecalsVector[1](fn=%d) Decals.size()=%u", frameNum, static_cast<uint32_t>(decals.size()));
	for (int cnt = 0; const auto & [owner, offset] : decalOwners) {
		const void* ptr = std::holds_alternative<const CSolidObject*>(owner) ? static_cast<const void*>(std::get<const CSolidObject*>(owner)) : nullptr;
		int id = (ptr != nullptr) ? std::get<const CSolidObject*>(owner)->id : -1;

		LOG("DH:CompactDecalsVector[1] [cnt=%d][ptr=%p][id=%d]=[pos=%u]",
			cnt++,
			ptr,
			id,
			static_cast<uint32_t>(offset)
		);
	}
#endif

	// temporary store the id --> DecalOwner relationship,
	// for the convinience to restore decalOwners correctness,
	// after the compaction is complete
	spring::unordered_map<uint32_t, DecalOwner> tmpOwnerToId;

	// Remove owners of expired items
	for (const auto& [owner, pos] : decalOwners) {
		assert(pos < decals.size());
		if (const auto& decal = decals[pos]; decal.IsValid()) {
			const uint32_t id = decal.info.id; //can't use bitfield directly below
			tmpOwnerToId.emplace(id, owner);
		}
	}

	// clean to restore it later
	decalOwners.clear();

	// group all expired items towards the end of the vector
	// Lua items are not considered expired
	const auto expIt = std::stable_partition(decals.begin(), decals.end(), [](const GroundDecal& decal) {
		return decal.IsValid();
	});

	// remove expired items from idToCmInfo map
	// and push expired ids to the freeIds vector
	for (auto decalIt = expIt; decalIt != decals.end(); ++decalIt) {
		freeIds.push_back(decalIt->info.id);

		if (auto it = idToCmInfo.find(decalIt->info.id); it != idToCmInfo.end())
			idToCmInfo.erase(it);
	}

	// remove expired decals
	decals.resize(decals.size() - numToDelete);
	decalsUpdateList.Resize(decals.size());

	idToPos.clear();
	for (size_t i = 0; i < decals.size(); ++i) {
		idToPos.emplace(decals[i].info.id, i);
	}

	// update the new positions of shrunk decals vector
	for (const auto& [id, owner] : tmpOwnerToId) {
		const auto ipIt = idToPos.find(id);
		if (ipIt == idToPos.end()) {
			assert(false);
			continue;
		}

		decalOwners.emplace(owner, ipIt->second);
	}


#if 0
	LOG("DH:CompactDecalsVector[2](fn=%d) Decals.size()=%u", frameNum, static_cast<uint32_t>(decals.size()));
	for (int cnt = 0; const auto & [owner, offset] : decalOwners) {
		const void* ptr = std::holds_alternative<const CSolidObject*>(owner) ? static_cast<const void*>(std::get<const CSolidObject*>(owner)) : nullptr;
		int id = (ptr != nullptr) ? std::get<const CSolidObject*>(owner)->id : -1;

		LOG("DH:CompactDecalsVector[2] [cnt=%d][ptr=%p][id=%d]=[pos=%u]",
			cnt++,
			ptr,
			id,
			static_cast<uint32_t>(offset)
		);
	}
#endif
}

void CGroundDecalHandler::UpdateDecalsVisibility()
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (const auto& [owner, pos] : decalOwners) {
		assert(pos < decals.size());
		auto& decal = decals[pos];

		if (decal.info.type != static_cast<uint8_t>(GroundDecal::Type::DECAL_PLATE))
			continue;

		if (std::holds_alternative<const CSolidObject*>(owner)) {
			const auto* so = std::get<const CSolidObject*>(owner);
			float wantedMult = 1.0f;

			if (const CUnit* unit = dynamic_cast<const CUnit*>(so); unit != nullptr) {
				const bool decalOwnerInCurLOS = ((unit->losStatus[gu->myAllyTeam] &   LOS_INLOS) != 0);
				const bool decalOwnerInPrvLOS = ((unit->losStatus[gu->myAllyTeam] & LOS_PREVLOS) != 0);
				const bool isGhostNow = gameSetup->ghostedBuildings && decalOwnerInPrvLOS && !decalOwnerInCurLOS;
				const bool iconOnly = (unit->GetDrawFlag() == DrawFlags::SO_DRICON_FLAG);

				if (!gu->spectatingFullView && isGhostNow) {
					// don't show ground decals for ghosts, this not for long used to be ghostDimming
					// borrowed from icons implementation, see https://github.com/beyond-all-reason/RecoilEngine/issues/2875
					wantedMult = 0.0f;
				}
				else if (iconOnly) {
					// icon -> hide decal (covers both spectating modes)
					wantedMult = 0.0f;
				}
				else if (!gu->spectatingFullView && !decalOwnerInCurLOS) {
					// out of LOS, !ghost, !spectatingFullView -> hide
					wantedMult = 0.0f;
				}

				wantedMult *= std::clamp(unit->buildProgress, 0.0f, 1.0f);
			}
			else {
				const CFeature* feature = static_cast<const CFeature*>(so);
				assert(feature);
				if (!feature->IsInLosForAllyTeam(gu->myAllyTeam))
					wantedMult = 0.0f;

				wantedMult = std::min(wantedMult, std::max(0.0f, feature->drawAlpha));
			}

			if (math::fabs(wantedMult - decal.visMult) > 0.05f) {
				decal.visMult = wantedMult;
				decalsUpdateList.SetUpdate(pos);
			}
		}
		else /* const GhostSolidObject* */ {
			const auto* gso = std::get<const GhostSolidObject*>(owner);
			// only display non-allied ghosts
			if (const auto gsoVis = !teamHandler.AlliedTeams(gu->myTeam, gso->team); !std::signbit(decal.visMult) != gsoVis) {
				decal.visMult = std::copysign(decal.visMult, 2.0f * gsoVis - 1.0f);
				decalsUpdateList.SetUpdate(pos);
			}
		}
	}

	const float curAdjustedFrame = std::max(gs->frameNum, 0) + globalRendering->timeOffset;
	for (auto& [id, info] : idToCmInfo) {
		auto it = idToPos.find(id);
		if (it == idToPos.end()) {
			assert(false);
			continue;
		}

		assert(it->second < decals.size());
		auto& decal = decals[it->second];

		const float currAlpha = decal.alpha - (curAdjustedFrame - decal.createFrameMax) * decal.alphaFalloff;
		if (currAlpha <= 0.0f)
			continue;

		const auto* cm = std::get<const CColorMap*>(info);
		auto idcs = cm->GetIndices(currAlpha);

		auto& storedIdcs = std::get<1>(info);
		if (storedIdcs == idcs)
			continue;

		decal.glowColorMap[0] = cm->GetColor(idcs.first );
		decal.glowColorMap[1] = cm->GetColor(idcs.second);
		storedIdcs = idcs;
		decalsUpdateList.SetUpdate(it->second);
	}
}

void CGroundDecalHandler::GameFramePost(int frameNum)
{
	RECOIL_DETAILED_TRACY_ZONE;
#if 0
	LOG("DH:GD(fn=%d) Decals.size()=%u", frameNum, static_cast<uint32_t>(decals.size()));
	for (int cnt = 0; const auto & [owner, offset] : decalOwners) {
		const void* ptr = std::holds_alternative<const CSolidObject*>(owner) ? static_cast<const void*>(std::get<const CSolidObject*>(owner)) : nullptr;
		int id = (ptr != nullptr) ? std::get<const CSolidObject*>(owner)->id : -1;

		LOG("DH:GD [cnt=%d][ptr=%p][id=%d]=[pos=%u]",
			cnt++,
			ptr,
			id,
			static_cast<uint32_t>(offset)
		);
	}
#endif
	// can't call AddTrack() directly in the loop below as it messes with decalOwners iteration order
	std::vector<const CUnit*> deferredTrackUpdate;

	for (const auto& [owner, _] : decalOwners) {
		if (!std::holds_alternative<const CSolidObject*>(owner))
			continue;

		const CUnit* unit = dynamic_cast<const CUnit*>(std::get<const CSolidObject*>(owner));
		if (unit == nullptr)
			continue;

		if (unit->moveType == nullptr)
			continue;

		if (unit->moveType->progressState == AMoveType::ProgressState::Active)
			continue;

		deferredTrackUpdate.emplace_back(unit);
	}

	for (const auto* unit : deferredTrackUpdate) {
		// call this one for stopped units, as AddTrack() is only called natively for moving units
		// This will be called several times before it's erased from decalOwners, not a big deal
		AddTrack(unit, unit->pos, true);
	}

	if (frameNum % 16 ==  0) {
		CompactDecalsVector(frameNum);
	}
}

void CGroundDecalHandler::SunChanged()
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto enToken = decalShader->EnableScoped();
	decalShader->SetUniform("groundAmbientColor", sunLighting->groundAmbientColor.x, sunLighting->groundAmbientColor.y, sunLighting->groundAmbientColor.z, sunLighting->groundShadowDensity);
	decalShader->SetUniform("groundDiffuseColor", sunLighting->groundDiffuseColor.x, sunLighting->groundDiffuseColor.y, sunLighting->groundDiffuseColor.z);
	decalShader->SetUniform3v("sunDir", &ISky::GetSky()->GetLight()->GetLightDir().x);
}

void CGroundDecalHandler::ViewResize()
{
	RECOIL_DETAILED_TRACY_ZONE;

	auto enToken = decalShader->EnableScoped();
	decalShader->SetUniform("screenSizeInverse",
		1.0f / globalRendering->viewSizeX,
		1.0f / globalRendering->viewSizeY
	);
}

void CGroundDecalHandler::GhostCreated(const CSolidObject* object, const GhostSolidObject* gb) { RemoveSolidObject(object, gb); }


void CGroundDecalHandler::ExplosionOccurred(const CExplosionParams& event) {
	if ((event.weaponDef != nullptr) && !event.weaponDef->visuals.explosionScar)
		return;

	const bool hasForcedProjVec = (event.weaponDef != nullptr && event.weaponDef->visuals.scarProjVector.w != 0.0f);
	const auto decalDir = hasForcedProjVec ?
		float3{ event.weaponDef->visuals.scarProjVector } :
		float3{ 0.0f, 0.0f, 0.0f };

	AddExplosion(std::move(AddExplosionInfo{
		event.pos,
		decalDir,
		event.damages.GetDefault(),
		event.craterAreaOfEffect,
		event.maxGroundDeformation,
		event.weaponDef
	}));
}

void CGroundDecalHandler::ConfigNotify(const std::string& key, const std::string& value)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (bool newHQ = configHandler->GetBool("HighQualityDecals") && (globalRendering->msaaLevel > 0); highQuality != newHQ) {
		sdbc = ScopedDepthBufferCopy(newHQ);
		highQuality = newHQ;
		ReloadDecalShaders();
	}
}

void CGroundDecalHandler::RenderUnitCreated(const CUnit* unit, int cloaked) { AddSolidObject(unit); }
void CGroundDecalHandler::RenderUnitDestroyed(const CUnit* unit) {
	if (auto it = unitMinMaxHeights.find(unit->id); it != unitMinMaxHeights.end())
		unitMinMaxHeights.erase(it);

	RemoveSolidObject(unit, nullptr);
}

void CGroundDecalHandler::RenderFeatureCreated(const CFeature* feature) { AddSolidObject(feature); }
void CGroundDecalHandler::RenderFeatureDestroyed(const CFeature* feature) { RemoveSolidObject(feature, nullptr); }
void CGroundDecalHandler::FeatureMoved(const CFeature* feature, const float3& oldpos) { MoveSolidObject(feature, feature->pos); }

// FIXME: Add a RenderUnitLoaded event
void CGroundDecalHandler::UnitLoaded(const CUnit* unit, const CUnit* transport) { ForceRemoveSolidObject(unit); }
void CGroundDecalHandler::UnitUnloaded(const CUnit* unit, const CUnit* transport) { AddSolidObject(unit); }

void CGroundDecalHandler::UnitMoved(const CUnit* unit) { AddTrack(unit, unit->pos); }