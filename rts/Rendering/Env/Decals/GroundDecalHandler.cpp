/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <cctype>

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
#include "Map/HeightMapTexture.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/DepthBufferCopy.h"
#include "Rendering/ShadowHandler.h"
#include "Rendering/Units/UnitDrawer.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/TexBind.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/GL/glHelpers.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Rendering/Textures/Bitmap.h"
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
#include "System/MemPoolTypes.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/FileSystem.h"

CONFIG(int, GroundScarAlphaFade).deprecated(true);
CONFIG(bool, HighQualityDecals).defaultValue(false).description("Forces MSAA processing of decals. Improves decals quality, but may ruin the performance.");

CGroundDecalHandler::CGroundDecalHandler()
	: CEventClient("[CGroundDecalHandler]", 314159, false)
	, maxUniqueScars{ 0 }
	, atlasMain{ nullptr }
	, atlasNorm{ nullptr }
	, decalShader{ nullptr }
	, decalsUpdateList{ }
	, smfDrawer { nullptr }
	, lastProcessedGameFrame{ std::numeric_limits<int>::lowest() }
{
	if (!GetDrawDecals())
		return;

	eventHandler.AddClient(this);
	CExplosionCreator::AddExplosionListener(this);

	configHandler->NotifyOnChange(this, { "HighQualityDecals" });
	highQuality = configHandler->GetBool("HighQualityDecals") && (globalRendering->msaaLevel > 0);
	depthBufferCopy->AddConsumer(highQuality, this);	

	smfDrawer = dynamic_cast<CSMFGroundDrawer*>(readMap->GetGroundDrawer());

	GenerateAtlasTextures();
	ReloadDecalShaders();

	instVBO = VBO(GL_ARRAY_BUFFER, false, false);

	decals.reserve(decalLevel * 16384);
	decalsUpdateList.Reserve(decals.capacity());
	GroundDecal::nextId = 0;
}

CGroundDecalHandler::~CGroundDecalHandler()
{
	CExplosionCreator::RemoveExplosionListener(this);
	eventHandler.RemoveClient(this);
	configHandler->RemoveObserver(this);

	depthBufferCopy->DelConsumer(highQuality, this);


	shaderHandler->ReleaseProgramObjects("[GroundDecalHandler]");
	decalShader = nullptr;
	atlasMain = nullptr;
	atlasNorm = nullptr;
}

static auto LoadTexture(const std::string& name, bool convertDecalBitmap)
{
	std::string fileName = StringToLower(name);

	if (FileSystem::GetExtension(fileName).empty())
		fileName += ".bmp";

	std::string fullName = fileName;

	if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL))
		fullName = std::string("bitmaps/") + fileName;

	if (!CFileHandler::FileExists(fullName, SPRING_VFS_ALL))
		fullName = std::string("unittextures/") + fileName;

	CBitmap bm;
	if (!bm.Load(fullName))
		throw content_error("Could not load ground decal \"" + fileName + "\"");

	if (convertDecalBitmap && FileSystem::GetExtension(fullName) == "bmp") {
		// bitmaps don't have an alpha channel
		// so use: red := brightness & green := alpha
		auto* rmem = bm.GetRawMem();

		for (int y = 0; y < bm.ysize; ++y) {
			for (int x = 0; x < bm.xsize; ++x) {
				const int index = ((y * bm.xsize) + x) * 4;

				const auto brightness = rmem[index + 0];
				const auto alpha      = rmem[index + 1];

				rmem[index + 0] = (brightness * 90) / 255;
				rmem[index + 1] = (brightness * 60) / 255;
				rmem[index + 2] = (brightness * 30) / 255;
				rmem[index + 3] = alpha;
			}
		}
	}
	// non BMP scar textures doesn't follow the above historic convention, so keep them as is

	return std::make_tuple(bm, fullName);
}

static inline std::string GetExtraTextureName(const std::string& mainTex) {
	auto dotPos = mainTex.find_last_of(".");
	return mainTex.substr(0, dotPos) + "_normal" + (dotPos == string::npos ? "" : mainTex.substr(dotPos));
}

void CGroundDecalHandler::AddTexToAtlas(const std::string& name, const std::string& filename, bool mainTex, bool convertOldBMP) {
	try {
		const auto& [bm, fn] = LoadTexture(filename, convertOldBMP);
		const auto& decalAtlas = (mainTex ? atlasMain : atlasNorm);
		decalAtlas->AddTexFromBitmap(name, bm);
	}
	catch (const content_error& err) {
		LOG_L(L_ERROR, "%s", err.what());
	}
}

void CGroundDecalHandler::AddBuildingDecalTextures()
{
	auto CreateFallBackTexture = [](const SColor& color) {
		CBitmap bm;
		bm.AllocDummy(color);
		bm = bm.CreateRescaled(32, 32);
		return bm.CreateTexture();
	};

	auto ProcessDefs = [this](const auto& defsVector) {
		for (const SolidObjectDef& soDef : defsVector) {
			const SolidObjectDecalDef& decalDef = soDef.decalDef;

			if (!decalDef.useGroundDecal)
				continue;

			if (decalDef.groundDecalTypeName.empty())
				continue;

			const std::string mainTex =                    (decalDef.groundDecalTypeName);
			const std::string normTex = GetExtraTextureName(decalDef.groundDecalTypeName);

			AddTexToAtlas(mainTex, mainTex, true , false);
			AddTexToAtlas(normTex, normTex, false, false);
		}
	};
	ProcessDefs(featureDefHandler->GetFeatureDefsVec());
	ProcessDefs(unitDefHandler->GetUnitDefsVec());
}


void CGroundDecalHandler::AddGroundScarTextures()
{
	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		LOG_L(L_ERROR, "Failed to load resources: %s", resourcesParser.GetErrorLog().c_str());
	}

	const LuaTable scarsTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("scars");
	const int scarTblSize = scarsTable.GetLength();

	maxUniqueScars = 0;
	for (int i = 1; i <= scarTblSize; ++i) {
		const std::string mainTexFileName = scarsTable.GetString(i, "");

		if (mainTexFileName.find("_normal") != std::string::npos)
			continue;

		const std::string normTexFileName = mainTexFileName.empty() ? "" : GetExtraTextureName(mainTexFileName);
		const auto mainName = IntToString(i, "mainscar_%i");
		const auto normName = IntToString(i, "normscar_%i");

		AddTexToAtlas(mainName, mainTexFileName,  true,  true);
		AddTexToAtlas(normName, normTexFileName, false, false);

		// check if loaded for real
		// can't use atlas->TextureExists() as it's only populated after Finalize()
		maxUniqueScars += atlasMain->GetAllocator()->contains(mainName);
	}

	if (maxUniqueScars == scarTblSize)
		return;

	const size_t scarsDeficit = static_cast<size_t>(scarTblSize - maxUniqueScars);

	const std::vector<std::string> scarMainTextures = CFileHandler::FindFiles("bitmaps/scars/", "scar?.*");
	const size_t scarsExtraNum = scarMainTextures.size();

	for (size_t i = 0; i < std::min(scarsDeficit, scarsExtraNum); ++i) {
		const std::string mainTexFileName = scarMainTextures[i];
		const std::string normTexFileName = GetExtraTextureName(mainTexFileName);
		const auto mainName = IntToString(maxUniqueScars + 1, "mainscar_%i");
		const auto normName = IntToString(maxUniqueScars + 1, "normscar_%i");

		AddTexToAtlas(mainName, mainTexFileName,  true,  true);
		AddTexToAtlas(normName, normTexFileName, false, false);

		// check if loaded for real
		// can't use atlas->TextureExists() as it's only populated after Finalize()
		maxUniqueScars += atlasMain->GetAllocator()->contains(mainName);
	}
}

void CGroundDecalHandler::AddGroundTrackTextures()
{
	const auto fileNames = CFileHandler::FindFiles("bitmaps/tracks/", "");
	for (const auto& mainTexFileName : fileNames) {
		const auto mainName = FileSystem::GetBasename(StringToLower(mainTexFileName));
		const auto normName = mainName + "_norm";
		const std::string normTexFileName = GetExtraTextureName(mainTexFileName);

		AddTexToAtlas(mainName, mainTexFileName,  true,  true);
		AddTexToAtlas(normName, normTexFileName, false, false);
	}
}


void CGroundDecalHandler::AddFallbackTextures()
{
	{
		const auto minDim = std::max(atlasMain->GetMinDim(), 32);
		atlasMain->AddTex("%FB_MAIN%", minDim, minDim, SColor(255,   0,   0, 255));
	}
	{
		const auto minDim = std::max(atlasNorm->GetMinDim(), 32);
		atlasNorm->AddTex("%FB_NORM%", minDim, minDim, SColor(128, 128, 255,   0));
	}
}

void CGroundDecalHandler::BindVertexAtrribs()
{
	for (int i = 0; i <= 6; ++i) {
		glEnableVertexAttribArray(i);
		glVertexAttribDivisor(i, 1);
	}

	// posTL, posTR
	glVertexAttribPointer(0, 4, GL_FLOAT, false, sizeof(GroundDecal), (const void*)offsetof(GroundDecal, posTL));
	// posBR, posBL
	glVertexAttribPointer(1, 4, GL_FLOAT, false, sizeof(GroundDecal), (const void*)offsetof(GroundDecal, posBR));
	// texMainOffsets
	glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(GroundDecal), (const void*)offsetof(GroundDecal, texMainOffsets));
	// texNormOffsets
	glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(GroundDecal), (const void*)offsetof(GroundDecal, texNormOffsets));
	// alpha, alphaFalloff, rot, height
	glVertexAttribPointer(4, 4, GL_FLOAT, false, sizeof(GroundDecal), (const void*)offsetof(GroundDecal, alpha));
	// createFrameMin, createFrameMax, uvWrapDistance, uvTraveledDistance
	glVertexAttribPointer(5, 4, GL_FLOAT, false, sizeof(GroundDecal), (const void*)offsetof(GroundDecal, createFrameMin));
	// forcedNormal, visMult
	glVertexAttribPointer(6, 4, GL_FLOAT, false, sizeof(GroundDecal), (const void*)offsetof(GroundDecal, forcedNormal));
}

void CGroundDecalHandler::UnbindVertexAtrribs()
{
	for (int i = 0; i <= 6; ++i) {
		glDisableVertexAttribArray(i);
		glVertexAttribDivisor(i, 0);
	}
}

uint32_t CGroundDecalHandler::GetDepthBufferTextureTarget() const
{
	return highQuality ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
}

static constexpr CTextureAtlas::AllocatorType defAllocType = CTextureAtlas::ATLAS_ALLOC_LEGACY;
static constexpr int defNumLevels = 4;
void CGroundDecalHandler::GenerateAtlasTextures() {
	atlasMain = std::make_unique<CTextureRenderAtlas>(defAllocType, 0, 0, GL_RGBA8, "DecalsMain");
	atlasNorm = std::make_unique<CTextureRenderAtlas>(defAllocType, 0, 0, GL_RGBA8, "DecalsNorm");

	atlasMain->SetMaxTexLevel(defNumLevels);
	atlasNorm->SetMaxTexLevel(defNumLevels);

	// often represented by compressed textures, cannot be added to the regular atlas
	AddBuildingDecalTextures();

	AddGroundScarTextures();
	AddGroundTrackTextures();
	AddFallbackTextures();

	if (!atlasMain->Finalize()) {
		LOG_L(L_ERROR, "Could not finalize %s texture atlas. Use fewer/smaller textures.", atlasMain->GetAtlasName().c_str());
	}

	if (!atlasNorm->Finalize()) {
		LOG_L(L_ERROR, "Could not finalize %s texture atlas. Use fewer/smaller textures.", atlasNorm->GetAtlasName().c_str());
	}
}

void CGroundDecalHandler::ReloadDecalShaders() {
	if (shaderHandler->ReleaseProgramObjects("[GroundDecalHandler]"))
		decalShader = nullptr;

	const std::string ver = highQuality ? "#version 400 compatibility\n" : "#version 130\n";

	decalShader = shaderHandler->CreateProgramObject("[GroundDecalHandler]", "DecalShaderGLSL");

	decalShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/GroundDecalsVertProg.glsl",  "", GL_VERTEX_SHADER));
	decalShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/GroundDecalsFragProg.glsl", ver, GL_FRAGMENT_SHADER));

	decalShader->SetFlag("DEPTH_CLIP01", globalRendering->supportClipSpaceControl);
	decalShader->SetFlag("HAVE_SHADOWS", true);
	decalShader->SetFlag("HIGH_QUALITY", highQuality);
	decalShader->SetFlag("HAVE_INFOTEX", true);

	decalShader->BindAttribLocation("posT"                    , 0);
	decalShader->BindAttribLocation("posB"                    , 1);
	decalShader->BindAttribLocation("uvMain"                  , 2);
	decalShader->BindAttribLocation("uvNorm"                  , 3);
	decalShader->BindAttribLocation("info"                    , 4);
	decalShader->BindAttribLocation("createParams"            , 5);
	decalShader->BindAttribLocation("forcedNormalAndAlphaMult", 6);

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

	decalShader->SetUniform("decalMainTex"   , 0);
	decalShader->SetUniform("decalNormTex"   , 1);
	decalShader->SetUniform("miniMapTex"     , 2);
	decalShader->SetUniform("heightTex"      , 3);
	decalShader->SetUniform("depthTex"       , 4);
	decalShader->SetUniform("groundNormalTex", 5);
	decalShader->SetUniform("shadowTex"      , 6);
	decalShader->SetUniform("shadowColorTex" , 7);
	decalShader->SetUniform("infoTex"        , 8);

	decalShader->SetUniform("curAdjustedFrame", std::max(gs->frameNum, 0) + globalRendering->timeOffset);
	decalShader->SetUniform("screenSizeInverse",
		1.0f / globalRendering->viewSizeX,
		1.0f / globalRendering->viewSizeY
	);
	const auto& identityMat = CMatrix44f::Identity();
	decalShader->SetUniformMatrix4x4("shadowMatrix", false, &identityMat.m[0]);

	decalShader->Disable();
	SunChanged();

	decalShader->Validate();
}

void CGroundDecalHandler::BindAtlasTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(atlasMain->GetTexTarget(), atlasMain->GetTexID());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(atlasNorm->GetTexTarget(), atlasNorm->GetTexID());
}

void CGroundDecalHandler::BindCommonTextures()
{
	const CSMFReadMap* smfMap = smfDrawer->GetReadMap();

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetMiniMapTexture());

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());

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
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(atlasMain->GetTexTarget(), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(atlasNorm->GetTexTarget(), 0);

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


void CGroundDecalHandler::AddExplosion(float3 pos, float damage, float radius)
{
	if (!GetDrawDecals())
		return;

	if (maxUniqueScars == 0)
		return;

	const float altitude = pos.y - CGround::GetHeightReal(pos.x, pos.z, false);
	const float3 groundNormal = CGround::GetNormal(pos.x, pos.z, false);

	// no decals for below-ground explosions
	// also no decals if they are too high in the air
	if (math::fabs(altitude) >= radius)
		return;

	pos.y -= altitude;
	radius -= altitude;

	if (radius < 5.0f)
		return;

	damage = std::min(damage, radius * 30.0f);
	damage *= (radius / (radius + altitude));
	radius = std::min(radius, damage * 0.25f);

	if (damage > 400.0f)
		damage = 400.0f + std::sqrt(damage - 400.0f);

	const int ttl = std::clamp(decalLevel * damage * 3.0f, 15.0f, decalLevel * 1800.0f);
	float alpha = std::clamp(2.0f * damage / 255.0f, 0.2f, 1.1f); // cap min alpha and glow
	float alphaDecay = 1.0f / ttl;

	float size = radius * math::SQRT2;

	const float2 posTL = { pos.x - size, pos.z - size };
	const float2 posTR = { pos.x + size, pos.z - size };
	const float2 posBR = { pos.x + size, pos.z + size };
	const float2 posBL = { pos.x - size, pos.z + size };

	const SRectangle newRect(
		math::floor(posTL.x),
		math::floor(posTL.y),
		math::ceil(posBR.x),
		math::ceil(posBR.y)
	);

	for (const auto& decal : decals) {
		if (decal.info.type != GroundDecal::Type::DECAL_EXPLOSION)
			continue;

		const float oldRad = 0.5f * (decal.posBR.x - decal.posTL.x) / math::SQRT2;

		if (radius > oldRad * 0.8f)
			continue;

		const SRectangle oldRect(
			math::floor(decal.posTL.x),
			math::floor(decal.posTL.y),
			math::ceil(decal.posBR.x),
			math::ceil(decal.posBR.y)
		);

		if (oldRect.Inside(newRect)) {
			alpha = std::max(alpha, decal.alpha);
			alphaDecay = std::min(alphaDecay, decal.alphaFalloff);
		}
	}

	const int scarIdx = 1 + guRNG.NextInt(maxUniqueScars); //not inclusive
	const auto mainName = IntToString(scarIdx, "mainscar_%i");
	const auto normName = IntToString(scarIdx, "normscar_%i");

	const auto createFrame = static_cast<float>(std::max(gs->frameNum, 0));

	const auto& decal = decals.emplace_back(GroundDecal{
		.posTL = posTL,
		.posTR = posTR,
		.posBR = posBR,
		.posBL = posBL,
		.texMainOffsets = atlasMain->GetTexture(mainName, "%FB_MAIN%"),
		.texNormOffsets = atlasNorm->GetTexture(normName, "%FB_NORM%"),
		.alpha = alpha,
		.alphaFalloff = alphaDecay,
		.rot = guRNG.NextFloat() * math::TWOPI,
		.height = size,
		.createFrameMin = createFrame,
		.createFrameMax = createFrame,
		.uvWrapDistance = 0.0f,
		.uvTraveledDistance = 0.0f,
		.forcedNormal = float3{},
		.visMult = 1.0f,
		.info = GroundDecal::TypeID{ .type = GroundDecal::Type::DECAL_EXPLOSION, .id = GroundDecal::GetNextId() }
	});

	idToPos.emplace(decal.info.id, decals.size() - 1);
	decalsUpdateList.EmplaceBackUpdate();
}

void CGroundDecalHandler::ReloadTextures()
{
	struct AtlasedTextureHash {
		uint32_t operator()(const AtlasedTexture& at) const {
			return spring::LiteHash(&at, sizeof(at));
		}
	};

	spring::unordered_map<AtlasedTexture, std::string, AtlasedTextureHash> subTexToNameMain;
	spring::unordered_map<AtlasedTexture, std::string, AtlasedTextureHash> subTexToNameNorm;

	for (const auto& [name, ae] : atlasMain->GetAllocator()->GetEntries()) {
		subTexToNameMain.emplace(atlasMain->GetTexture(name), name);
	}
	for (const auto& [name, ae] : atlasNorm->GetAllocator()->GetEntries()) {
		subTexToNameNorm.emplace(atlasNorm->GetTexture(name), name);
	}

	// can't use {atlas}->ReloadTextures() here as all textures come from memory
	atlasMain = nullptr;
	atlasNorm = nullptr;
	GenerateAtlasTextures();

	for (auto& decal : decals) {
		if (auto it = subTexToNameMain.find(decal.texMainOffsets); it != subTexToNameMain.end()) {
			decal.texMainOffsets = atlasMain->GetTexture(it->second, "%FB_MAIN%");
		}
		if (auto it = subTexToNameNorm.find(decal.texNormOffsets); it != subTexToNameNorm.end()) {
			decal.texNormOffsets = atlasNorm->GetTexture(it->second, "%FB_NORM%");
		}
	}
	decalsUpdateList.SetNeedUpdateAll();
}

void CGroundDecalHandler::DumpAtlasTextures()
{
	atlasMain->DumpTexture();
	atlasNorm->DumpTexture();
}

void CGroundDecalHandler::Draw()
{
	if (!GetDrawDecals())
		return;

	if (!smfDrawer->UseAdvShading())
		return;

	if (decals.empty())
		return;

	if (!atlasMain->IsValid() || !atlasNorm->IsValid())
		return;

	UpdateDecalsVisibility();

	if (instVBO.GetSize() < decals.size() * sizeof(GroundDecal)) {
		vao.Bind();

		instVBO.Bind();
		instVBO.New(decals.capacity() * sizeof(GroundDecal), GL_STREAM_DRAW);
		BindVertexAtrribs();

		vao.Unbind();

		UnbindVertexAtrribs();
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
		Blending(GL_TRUE),
		BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA),
		DepthMask(GL_FALSE),
		DepthTest(GL_FALSE),
		Culling(GL_TRUE),
		CullFace(GL_BACK)
	);

	BindCommonTextures();
	BindAtlasTextures();

	decalShader->SetFlag("HAVE_SHADOWS", shadowHandler.ShadowsLoaded());
	decalShader->SetFlag("HAVE_INFOTEX", infoTextureHandler->IsEnabled());
	decalShader->Enable();

	decalShader->SetUniform("infoTexIntensityMul", float(infoTextureHandler->InMetalMode()) + 1.0f);
	decalShader->SetUniform("curAdjustedFrame", std::max(gs->frameNum, 0) + globalRendering->timeOffset);
	if (shadowHandler.ShadowsLoaded())
		decalShader->SetUniformMatrix4x4("shadowMatrix", false, shadowHandler.GetShadowMatrixRaw());

	if (!decals.empty()) {
		vao.Bind();
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, decals.size());
		vao.Unbind();
	}

	decalShader->Disable();

	UnbindTextures();
}

void CGroundDecalHandler::AddSolidObject(const CSolidObject* object) { MoveSolidObject(object, object->pos); }
void CGroundDecalHandler::MoveSolidObject(const CSolidObject* object, const float3& pos)
{
	if (!GetDrawDecals())
		return;

	const SolidObjectDecalDef& decalDef = object->GetDef()->decalDef;

	if (!decalDef.useGroundDecal || decalDef.groundDecalTypeName.empty())
		return;

	int sizex = decalDef.groundDecalSizeX * SQUARE_SIZE;
	int sizey = decalDef.groundDecalSizeY * SQUARE_SIZE;

	// swap xsize and ysize if object faces East or West
	if (object->buildFacing == FACING_EAST || object->buildFacing == FACING_WEST)
		std::swap(sizex, sizey);

	const float2 midPoint = float2(static_cast<int>(pos.x / SQUARE_SIZE), static_cast<int>(pos.z / SQUARE_SIZE)) * SQUARE_SIZE;
	const float midPointHeight = CGround::GetHeightReal(midPoint.x, midPoint.y);

	const auto posTL = midPoint + float2(-sizex, -sizey);
	const auto posTR = midPoint + float2( sizex, -sizey);
	const auto posBR = midPoint + float2( sizex,  sizey);
	const auto posBL = midPoint + float2(-sizex,  sizey);

	const float height = argmax(
		math::fabs(midPointHeight - CGround::GetHeightReal(posTL.x, posTL.y)),
		math::fabs(midPointHeight - CGround::GetHeightReal(posTR.x, posTR.y)),
		math::fabs(midPointHeight - CGround::GetHeightReal(posBR.x, posBR.y)),
		math::fabs(midPointHeight - CGround::GetHeightReal(posBL.x, posBL.y))
	) + 1.0f;

	const auto createFrame = static_cast<float>(std::max(gs->frameNum, 0));

	if (const auto doIt = decalOwners.find(object); doIt != decalOwners.end()) {
		auto& decal = decals.at(doIt->second);
		decal.posTL = posTL;
		decal.posTR = posTR;
		decal.posBR = posBR;
		decal.posBL = posBL;
		decal.height = height;
		decalsUpdateList.SetUpdate(doIt->second);
		return;
	}

	const auto& decal = decals.emplace_back(GroundDecal{
		.posTL = posTL,
		.posTR = posTR,
		.posBR = posBR,
		.posBL = posBL,
		.texMainOffsets = atlasMain->GetTexture(                   (decalDef.groundDecalTypeName), "%FB_MAIN%"),
		.texNormOffsets = atlasNorm->GetTexture(GetExtraTextureName(decalDef.groundDecalTypeName), "%FB_NORM%"),
		.alpha = 1.0f,
		.alphaFalloff = 0.0f,
		.rot = 0.0f,
		.height = height,
		.createFrameMin = createFrame,
		.createFrameMax = createFrame,
		.uvWrapDistance = 0.0f,
		.uvTraveledDistance = 0.0f,
		.forcedNormal = float3{},
		.visMult = 1.0f,
		.info = GroundDecal::TypeID{.type = GroundDecal::Type::DECAL_PLATE, .id = GroundDecal::GetNextId() }
	});

	decalsUpdateList.EmplaceBackUpdate();
	idToPos.emplace(decal.info.id, decals.size() - 1);
	decalOwners.emplace(object, decals.size() - 1);
}

void CGroundDecalHandler::RemoveSolidObject(const CSolidObject* object, const GhostSolidObject* gb)
{
	assert(object);

	const auto doIt = decalOwners.find(object);
	if (doIt == decalOwners.end()) {
		// it's ok for an object to not have any decals
		return;
	}

	if (gb) {
		// gb is the new owner
		decalOwners.emplace(gb, doIt->second);
		decalOwners.erase(doIt);
		return;
	}

	auto& decayingDecal = decals.at(doIt->second);

	// we only care about DECAL_PLATE decals below
	if (decayingDecal.info.type != GroundDecal::Type::DECAL_PLATE) {
		decalOwners.erase(doIt);
		return;
	}

	const auto createFrame = static_cast<float>(std::max(gs->frameNum, 0));

	decayingDecal.alphaFalloff = object->GetDef()->decalDef.groundDecalDecaySpeed / GAME_SPEED;
	decayingDecal.createFrameMin = createFrame;
	decayingDecal.createFrameMax = createFrame;

	decalsUpdateList.SetUpdate(doIt->second);

	decalOwners.erase(doIt);
}

/**
 * @brief immediately remove an object's ground decal, if any (without fade out)
 */
void CGroundDecalHandler::ForceRemoveSolidObject(const CSolidObject* object)
{
	RemoveSolidObject(object, nullptr);
}

void CGroundDecalHandler::GhostDestroyed(const GhostSolidObject* gb) {
	const auto doIt = decalOwners.find(gb);
	if (doIt == decalOwners.end())
		return;

	auto& decal = decals.at(doIt->second);
	// just in case
	if (decal.info.type != GroundDecal::Type::DECAL_PLATE)
		return;
	
	decal.alpha = 0.0f;
	decalsUpdateList.SetUpdate(doIt->second);
	decalOwners.erase(doIt);
}

uint32_t CGroundDecalHandler::CreateLuaDecal()
{
	const auto createFrame = static_cast<float>(std::max(gs->frameNum, 0));

	const auto& decal = decals.emplace_back(GroundDecal{
		.posTL = float2{},
		.posTR = float2{},
		.posBR = float2{},
		.posBL = float2{},
		.texMainOffsets = atlasMain->GetTexture("%FB_MAIN%"),
		.texNormOffsets = atlasNorm->GetTexture("%FB_NORM%"),
		.alpha = 1.0f,
		.alphaFalloff = 0.0f,
		.rot = 0.0f,
		.height = 0.0f,
		.createFrameMin = createFrame,
		.createFrameMax = createFrame,
		.uvWrapDistance = 0.0f,
		.uvTraveledDistance = 0.0f,
		.forcedNormal = float3{},
		.visMult = 1.0f,
		.info = GroundDecal::TypeID{.type = GroundDecal::Type::DECAL_LUA, .id = GroundDecal::GetNextId() }
	});
	decalsUpdateList.EmplaceBackUpdate();
	idToPos.emplace(decal.info.id, decals.size() - 1);

	return decal.info.id;
}

bool CGroundDecalHandler::DeleteLuaDecal(uint32_t id)
{
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return false;

	auto& decal = decals.at(it->second);
	if (decal.info.type != GroundDecal::Type::DECAL_LUA)
		return false;

	decal.alpha = 0.0f;
	decalsUpdateList.SetUpdate(it->second);
	idToPos.erase(it); // the decal itself will be removed on the next compaction
	return true;
}

GroundDecal* CGroundDecalHandler::GetDecalById(uint32_t id)
{
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return nullptr;

	decalsUpdateList.SetUpdate(it->second);
	return &decals.at(it->second);
}

const GroundDecal* CGroundDecalHandler::GetDecalById(uint32_t id) const
{
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return nullptr;

	return &decals.at(it->second);
}

bool CGroundDecalHandler::SetDecalTexture(uint32_t id, const std::string& texName, bool mainTex)
{
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return false;

	auto& decal = decals.at(it->second);
	const auto& atlas  = mainTex ? atlasMain : atlasNorm;
	      auto& offset = mainTex ? decal.texMainOffsets : decal.texNormOffsets;

	const AtlasedTexture newOffset = atlas->GetTexture(texName);
	if (newOffset == AtlasedTexture::DefaultAtlasTexture)
		return false;

	offset = newOffset;
	return true;
}

std::string CGroundDecalHandler::GetDecalTexture(uint32_t id, bool mainTex) const
{
	auto it = idToPos.find(id);
	if (it == idToPos.end())
		return "";

	const auto& decal = decals.at(it->second);
	const auto& offset = mainTex ? decal.texMainOffsets : decal.texNormOffsets;
	const auto& atlas = mainTex ? atlasMain : atlasNorm;

	for (auto& [name, _] : atlas->GetAllocator()->GetEntries()) {
		const auto at = atlas->GetTexture(name);
		if (at == offset)
			return name;
	}

	return "";
}

const std::vector<std::string> CGroundDecalHandler::GetDecalTextures(bool mainTex) const
{
	const auto& atlas = mainTex ? atlasMain : atlasNorm;
	std::vector<std::string> ret;
	for (auto& [name, _] : atlas->GetAllocator()->GetEntries()) {
		ret.emplace_back(name);
	}
	std::sort(ret.begin(), ret.end());
	return ret;
}

const CSolidObject* CGroundDecalHandler::GetDecalSolidObjectOwner(uint32_t id) const
{
	for (const auto& [owner, pos] : decalOwners) {
		if (!std::holds_alternative<const CSolidObject*>(owner))
			continue;

		if (id != decals.at(pos).info.id)
			continue;

		return std::get<const CSolidObject*>(owner);
	}

	return nullptr;
}

static inline bool CanReceiveTracks(const float3& pos)
{
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

	const float trackLifeTime = decalLevel * GAME_SPEED * decalDef.trackDecalStrength;
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

		const auto& mainName = StringToLower(decalDef.trackDecalTypeName);
		const auto  normName = GetExtraTextureName(mainName);

		const float alphaDecay = 1.0f / trackLifeTime;

		const auto& decal = decals.emplace_back(GroundDecal{
			.posTL = decalPos2 - wc,
			.posTR = decalPos2 - wc,
			.posBR = decalPos2 + wc,
			.posBL = decalPos2 + wc,
			.texMainOffsets = atlasMain->GetTexture(mainName, "%FB_MAIN%"),
			.texNormOffsets = atlasNorm->GetTexture(normName, "%FB_NORM%"),
			.alpha = 1.0f,
			.alphaFalloff = alphaDecay,
			.rot = 0.0f,
			.height = 0.0f,
			.createFrameMin = createFrame,
			.createFrameMax = createFrame,
			.uvWrapDistance = decalDef.trackDecalWidth * decalDef.trackDecalStretch,
			.uvTraveledDistance = 0.0f,
			.forcedNormal = float3{unit->updir},
			.visMult = 1.0f,
			.info = GroundDecal::TypeID{.type = GroundDecal::Type::DECAL_TRACK, .id = GroundDecal::GetNextId() }
		});

		mm = {};

		decalOwners.emplace(unit, decals.size() - 1);
		idToPos.emplace(decal.info.id, decals.size() - 1);
		decalsUpdateList.EmplaceBackUpdate();

		return;
	}

	float decalHeight = CGround::GetHeightReal(decalPos.x, decalPos.z, false);
	mm.min = std::min(mm.min, decalHeight);
	mm.max = std::max(mm.max, decalHeight);

	if (!forceEval && createFrameInt % TRACKS_UPDATE_RATE != 0)
		return;

	GroundDecal& oldDecal = decals.at(doIt->second);

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
	if ((dirO.Dot(dirO) == 0.0f || dirO.Dot(dirN) >= 0.999f) && oldDecal.forcedNormal.dot(unit->updir) >= 0.95f) {
		oldDecal.posTR = decalPos2 - wc;
		oldDecal.posBR = decalPos2 + wc;
		oldDecal.createFrameMax = createFrame;

		const float2 midPointDist = (oldDecal.posTL + oldDecal.posTR + oldDecal.posBR + oldDecal.posBL) * 0.25f;
		const float midPointHeight = CGround::GetHeightReal(midPointDist.x, midPointDist.y, false);
		oldDecal.height = std::max(mm.max - midPointHeight, midPointHeight - mm.min) + 1.0f;

		decalsUpdateList.SetUpdate(doIt->second);
		return;
	}

	// new decal, starting where the previous ended
	auto& newDecal = decals.emplace_back(GroundDecal{
		.posTL = oldDecal.posTR,
		.posTR = decalPos2 - wc,
		.posBR = decalPos2 + wc,
		.posBL = oldDecal.posBR,
		.texMainOffsets = oldDecal.texMainOffsets,
		.texNormOffsets = oldDecal.texNormOffsets,
		.alpha = oldDecal.alpha,
		.alphaFalloff = oldDecal.alphaFalloff,
		.rot = 0.0f,
		.height = oldDecal.height, //also set later
		.createFrameMin = oldDecal.createFrameMax,
		.createFrameMax = createFrame,
		.uvWrapDistance = decalDef.trackDecalWidth * decalDef.trackDecalStretch,
		.uvTraveledDistance = oldDecal.uvTraveledDistance + posL.Distance(posR)/*oldDecal.posTL.Distance(oldDecal.posTR)*/,
		.forcedNormal = float3{ unit->updir },
		.visMult = 1.0f,
		.info = GroundDecal::TypeID{.type = GroundDecal::Type::DECAL_TRACK, .id = GroundDecal::GetNextId() }
	});

	const float2 midPointDist = (newDecal.posTL + newDecal.posTR + newDecal.posBR + newDecal.posBL) * 0.25f;
	const float midPointHeight = CGround::GetHeightReal(midPointDist.x, midPointDist.y, false);
	newDecal.height = std::max(newDecal.height, std::max(mm.max - midPointHeight, midPointHeight - mm.min)) + 1.0f;
	mm = {};

	// replace the old entry
	decalOwners[unit] = decals.size() - 1;

	idToPos.emplace(newDecal.info.id, decals.size() - 1);
	decalsUpdateList.EmplaceBackUpdate();
}

void CGroundDecalHandler::CompactDecalsVector(int frameNum)
{
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
		const auto targetExpirationFrame = static_cast<int>(decal.alpha / decal.alphaFalloff);
		if (frameNum - decal.createFrameMax > targetExpirationFrame) {
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

	// Remove owners of expired items
	for (auto doIt = decalOwners.begin(); doIt != decalOwners.end(); /*NOOP*/) {
		if (decals[doIt->second].IsValid())
			doIt = decalOwners.erase(doIt);
		else
			doIt++;
	}

	// group all expired items towards the end of the vector
	// Lua items are not considered expired
	auto partIt = std::stable_partition(decals.begin(), decals.end(), [](const GroundDecal& decal) {
		return decal.IsValid() || decal.info.type == GroundDecal::Type::DECAL_LUA;
	});

	// remove expired decals
	decals.resize(decals.size() - numToDelete);
	decalsUpdateList.Resize(decals.size());

	idToPos.clear();
	for (size_t i = 0; i < decals.size(); ++i) {
		idToPos.emplace(decals[i].info.id, i);
	}
}

void CGroundDecalHandler::UpdateDecalsVisibility()
{
	for (const auto& [owner, posType] : decalOwners) {
		auto& decal = decals.at(posType);

		if (decal.info.type != GroundDecal::Type::DECAL_PLATE)
			continue;

		if (std::holds_alternative<const CSolidObject*>(owner)) {
			const auto* so = std::get<const CSolidObject*>(owner);
			float wantedMult = 1.0f;

			if (const CUnit* unit = dynamic_cast<const CUnit*>(so); unit != nullptr) {
				const bool decalOwnerInCurLOS = ((unit->losStatus[gu->myAllyTeam] &   LOS_INLOS) != 0);
				const bool decalOwnerInPrvLOS = ((unit->losStatus[gu->myAllyTeam] & LOS_PREVLOS) != 0);

				if (unit->GetIsIcon())
					wantedMult = 0.0f;

				if (!gu->spectatingFullView && !decalOwnerInCurLOS && (!gameSetup->ghostedBuildings || !decalOwnerInPrvLOS))
					wantedMult = 0.0f;

				wantedMult = std::min(wantedMult, std::max(0.0f, unit->buildProgress));
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
				decalsUpdateList.SetUpdate(posType);
			}
		}
		else /* const GhostSolidObject* */ {
			////
		}
	}
}

void CGroundDecalHandler::GameFrame(int frameNum)
{
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
	auto enToken = decalShader->EnableScoped();
	decalShader->SetUniform("groundAmbientColor", sunLighting->groundAmbientColor.x, sunLighting->groundAmbientColor.y, sunLighting->groundAmbientColor.z, sunLighting->groundShadowDensity);
	decalShader->SetUniform("groundDiffuseColor", sunLighting->groundDiffuseColor.x, sunLighting->groundDiffuseColor.y, sunLighting->groundDiffuseColor.z);
	decalShader->SetUniform3v("sunDir", &ISky::GetSky()->GetLight()->GetLightDir().x);
}

void CGroundDecalHandler::ViewResize()
{
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

	AddExplosion(event.pos, event.damages.GetDefault(), event.craterAreaOfEffect);
}

void CGroundDecalHandler::ConfigNotify(const std::string& key, const std::string& value)
{
	if (key != "HighQualityDecals")
		return;

	if (bool newHQ = configHandler->GetBool("HighQualityDecals") && (globalRendering->msaaLevel > 0); highQuality != newHQ) {
		depthBufferCopy->DelConsumer(highQuality, this);
		depthBufferCopy->AddConsumer(      newHQ, this);
		highQuality = newHQ;
		ReloadDecalShaders();
	}
}

void CGroundDecalHandler::RenderUnitCreated(const CUnit* unit, int cloaked) { AddSolidObject(unit); }
void CGroundDecalHandler::RenderUnitDestroyed(const CUnit* unit) { RemoveSolidObject(unit, nullptr); }

void CGroundDecalHandler::RenderFeatureCreated(const CFeature* feature) { AddSolidObject(feature); }
void CGroundDecalHandler::RenderFeatureDestroyed(const CFeature* feature) { RemoveSolidObject(feature, nullptr); }
void CGroundDecalHandler::FeatureMoved(const CFeature* feature, const float3& oldpos) { MoveSolidObject(feature, feature->pos); }

// FIXME: Add a RenderUnitLoaded event
void CGroundDecalHandler::UnitLoaded(const CUnit* unit, const CUnit* transport) { ForceRemoveSolidObject(unit); }
void CGroundDecalHandler::UnitUnloaded(const CUnit* unit, const CUnit* transport) { AddSolidObject(unit); }

void CGroundDecalHandler::UnitMoved(const CUnit* unit) { AddTrack(unit, unit->pos); }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CGroundDecalHandler::DecalUpdateList::SetNeedUpdateAll()
{
	std::fill(updateList.begin(), updateList.end(), true);
	changed = true;
}

void CGroundDecalHandler::DecalUpdateList::ResetNeedUpdateAll()
{
	std::fill(updateList.begin(), updateList.end(), false);
	changed = false;
}

void CGroundDecalHandler::DecalUpdateList::SetUpdate(const CGroundDecalHandler::DecalUpdateList::IteratorPair& it)
{
	std::fill(it.first, it.second, true);
	changed = true;
}

void CGroundDecalHandler::DecalUpdateList::SetUpdate(size_t offset)
{
	assert(offset < updateList.size());
	updateList[offset] = true;
	changed = true;
}

void CGroundDecalHandler::DecalUpdateList::EmplaceBackUpdate()
{
	updateList.emplace_back(true);
	changed = true;
}

std::optional<CGroundDecalHandler::DecalUpdateList::IteratorPair> CGroundDecalHandler::DecalUpdateList::GetNext(const std::optional<CGroundDecalHandler::DecalUpdateList::IteratorPair>& prev)
{
	auto beg = prev.has_value() ? prev.value().second : updateList.begin();
	     beg = std::find(beg, updateList.end(),  true);
	auto end = std::find(beg, updateList.end(), false);

	if (beg == end)
		return std::nullopt;

	return std::make_optional(std::make_pair(beg, end));
}

std::pair<size_t, size_t> CGroundDecalHandler::DecalUpdateList::GetOffsetAndSize(const CGroundDecalHandler::DecalUpdateList::IteratorPair& it)
{
	return std::make_pair(
		std::distance(updateList.begin(), it.first ),
		std::distance(it.first          , it.second)
	);
}
