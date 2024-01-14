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
#include "Rendering/GL/VertexArray.h"
#include "Rendering/GL/SubState.h"
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
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
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
	, atlas{ nullptr }
	, decalShader{ nullptr }
	, tmpNeedsUpdate{ true }
	, permNeedsUpdate{ true }
	, smfDrawer { nullptr }
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

	instTmpVBO = VBO(GL_ARRAY_BUFFER, false, false);

	temporaryDecals.reserve(decalLevel * 16384);
	tmpUpdateIndicator.reserve(temporaryDecals.capacity());
	permanentDecals.reserve(8192);

	GroundDecal::nextId = 0u;
}

CGroundDecalHandler::~CGroundDecalHandler()
{
	CExplosionCreator::RemoveExplosionListener(this);
	eventHandler.RemoveClient(this);
	configHandler->RemoveObserver(this);

	depthBufferCopy->DelConsumer(highQuality, this);


	shaderHandler->ReleaseProgramObjects("[GroundDecalHandler]");
	decalShader = nullptr;
	atlas = nullptr;
	groundDecalAtlasMain = nullptr;
	groundDecalAtlasNorm = nullptr;
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

void CGroundDecalHandler::AddTexToGroundAtlas(const std::string& name, bool mainTex) {
	try {
		const auto& [bm, fn] = LoadTexture(name, mainTex);
		const auto& groundDecalAtlas = (mainTex ? groundDecalAtlasMain : groundDecalAtlasNorm);
		groundDecalAtlas->AddTexFromBitmap(name, bm);
	}
	catch (const content_error& err) {
		LOG_L(L_ERROR, "%s", err.what());
	}
}

void CGroundDecalHandler::AddTexToAtlas(const std::string& name, bool mainTex) {
	try {
		const auto& [bm, fn] = LoadTexture(name, mainTex);
		atlas->AddTexFromMem(name, bm.xsize, bm.ysize, CTextureAtlas::RGBA32, bm.GetRawMem());
	}
	catch (const content_error& err) {
		LOG_L(L_ERROR, "%s", err.what());
	}
}

void CGroundDecalHandler::AddTexToAtlas(const std::string& name, const std::string& filename, const std::string& filenameAlt, bool mainTex) {
	if (!filename.empty())
	try {
		const auto& [bm, fn] = LoadTexture(filename, mainTex);
		atlas->AddTexFromMem(name, bm.xsize, bm.ysize, CTextureAtlas::RGBA32, bm.GetRawMem());
		return;
	}
	catch (const content_error& err) {
		LOG_L(L_ERROR, "%s", err.what());
	}

	if (!filenameAlt.empty())
	try {
		const auto& [bm, fn] = LoadTexture(filenameAlt, mainTex);
		atlas->AddTexFromMem(name, bm.xsize, bm.ysize, CTextureAtlas::RGBA32, bm.GetRawMem());
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

			AddTexToGroundAtlas(                   (decalDef.groundDecalTypeName), true );
			AddTexToGroundAtlas(GetExtraTextureName(decalDef.groundDecalTypeName), false);
		}
	};
	ProcessDefs(featureDefHandler->GetFeatureDefsVec());
	ProcessDefs(unitDefHandler->GetUnitDefsVec());

	{
		const auto minDim = std::max(groundDecalAtlasMain->GetMinDim(), 32);
		groundDecalAtlasMain->AddTex("%FB_MAIN%", minDim, minDim, SColor(255,   0,   0, 255));
	}
	{
		const auto minDim = std::max(groundDecalAtlasNorm->GetMinDim(), 32);
		groundDecalAtlasNorm->AddTex("%FB_NORM%", minDim, minDim, SColor(128, 128, 255, 128));
	}
}


void CGroundDecalHandler::AddGroundScarTextures()
{
	LuaParser resourcesParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser.Execute()) {
		LOG_L(L_ERROR, "Failed to load resources: %s", resourcesParser.GetErrorLog().c_str());
	}

	const std::vector<std::string> scarMainTextures = CFileHandler::FindFiles("bitmaps/scars/", "scar?.*");

	const LuaTable scarsTable = resourcesParser.GetRoot().SubTable("graphics").SubTable("scars");

	for (int N = scarsTable.GetLength(), i = 1; i <= N; ++i) {
		const std::string mainTexFileName = scarsTable.GetString(i, "");
		const std::string normTexFileName = mainTexFileName.empty() ? "" : GetExtraTextureName(mainTexFileName);
		const std::string mainTexFileNameAlt = scarMainTextures[(i - 1) % scarMainTextures.size()];
		const std::string normTexFileNameAlt = GetExtraTextureName(mainTexFileNameAlt);
		const auto mainName = IntToString(i, "mainscar_%i");
		const auto normName = IntToString(i, "normscar_%i");

		AddTexToAtlas(mainName, mainTexFileName, mainTexFileNameAlt, true );
		AddTexToAtlas(normName, normTexFileName, normTexFileNameAlt, false);
	}
}

void CGroundDecalHandler::AddGroundTrackTextures()
{
	const auto fileNames = CFileHandler::FindFiles("bitmaps/tracks/", "");
	for (const auto& mainTexFileName : fileNames) {
		const auto mainName = FileSystem::GetBasename(StringToLower(mainTexFileName));
		const auto normName = mainName + "_norm";
		const std::string normTexFileName = GetExtraTextureName(mainTexFileName);

		AddTexToAtlas(mainName, mainTexFileName, "",  true);
		AddTexToAtlas(normName, normTexFileName, "", false);
	}
}


void CGroundDecalHandler::AddFallbackTextures()
{
	auto CreateFallBack = [](const SColor& color) {
		CBitmap bm;
		bm.AllocDummy(color);
		bm = bm.CreateRescaled(32, 32);
		return std::make_tuple(bm, int2(bm.xsize, bm.ysize));
	};
	{
		const auto& [bm, dims] = CreateFallBack(SColor(255,   0,   0, 255)); // RGB + alpha
		atlas->AddTexFromMem("%FB_MAIN%", dims.x, dims.y, CTextureAtlas::RGBA32, bm.GetRawMem());
	}
	{
		const auto& [bm, dims] = CreateFallBack(SColor(128, 128, 255, 128)); // normal + glow
		atlas->AddTexFromMem("%FB_NORM%", dims.x, dims.y, CTextureAtlas::RGBA32, bm.GetRawMem());
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
	// createFrameMin, createFrameMax
	glVertexAttribIPointer(5, 2, GL_UNSIGNED_INT, sizeof(GroundDecal), (const void*)offsetof(GroundDecal, createFrameMin));
	// uvWrapDistance
	glVertexAttribPointer(6, 2, GL_FLOAT, false, sizeof(GroundDecal), (const void*)offsetof(GroundDecal, uvWrapDistance));
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

void CGroundDecalHandler::GenerateAtlasTextures() {
	atlas = std::make_unique<CTextureAtlas>(CTextureAtlas::ATLAS_ALLOC_QUADTREE, 0, 0, "DecalTextures", true);
	groundDecalAtlasMain = std::make_unique<TextureRenderAtlas>(CTextureAtlas::ATLAS_ALLOC_QUADTREE, 0, 0, GL_RGBA8, "BuildingDecalsMain");
	groundDecalAtlasNorm = std::make_unique<TextureRenderAtlas>(CTextureAtlas::ATLAS_ALLOC_QUADTREE, 0, 0, GL_RGBA8, "BuildingDecalsNorm");

	// often represented by compressed textures, cannot be added to the atlas
	AddBuildingDecalTextures();

	AddGroundScarTextures();
	AddGroundTrackTextures();
	AddFallbackTextures();

	bool b =
		groundDecalAtlasMain->Finalize() &&
		groundDecalAtlasNorm->Finalize() &&
		atlas->Finalize();

	//atlas->DumpTexture("CGroundDecalHandler.bmp");
	assert(b);

	for (int i = 1; /*NOOP*/; ++i) {
		if (!atlas->TextureExists(IntToString(i, "mainscar_%i"))) {
			maxUniqueScars = i - 1;
			break;
		}
	}

	glBindTexture(GL_TEXTURE_2D, atlas->GetTexID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
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
	decalShader->SetFlag("HAVE_MULTISAMPLING", highQuality);

	decalShader->BindAttribLocation("posT"       , 0);
	decalShader->BindAttribLocation("posB"       , 1);
	decalShader->BindAttribLocation("uvMain"     , 2);
	decalShader->BindAttribLocation("uvNorm"     , 3);
	decalShader->BindAttribLocation("info"       , 4);
	decalShader->BindAttribLocation("createFrame", 5);
	decalShader->BindAttribLocation("uvParams"   , 6);

	decalShader->Link();

	decalShader->Enable();
	decalShader->SetUniform("mapDims",
		static_cast<float>(mapDims.mapx * SQUARE_SIZE),
		static_cast<float>(mapDims.mapy * SQUARE_SIZE),
		1.0f / (mapDims.mapx * SQUARE_SIZE),
		1.0f / (mapDims.mapy * SQUARE_SIZE)
	);
	decalShader->SetUniform("decalMainTex", 0);
	decalShader->SetUniform("decalNormTex", 1);
	decalShader->SetUniform("shadeTex", 2);
	decalShader->SetUniform("heightTex", 3);
	decalShader->SetUniform("depthTex", 4);

	decalShader->SetUniform("groundNormalTex", 5);
	decalShader->SetUniform("shadowTex", 6);
	decalShader->SetUniform("shadowColorTex", 7);
	decalShader->SetUniform("groundAmbientColor", sunLighting->groundAmbientColor.x, sunLighting->groundAmbientColor.y, sunLighting->groundAmbientColor.z, sunLighting->groundShadowDensity);
	decalShader->SetUniform("groundDiffuseColor", sunLighting->groundDiffuseColor.x, sunLighting->groundDiffuseColor.y, sunLighting->groundDiffuseColor.z);

	decalShader->SetUniform("curAdjustedFrame", std::max(gs->frameNum, 0) + globalRendering->timeOffset);
	decalShader->SetUniform("screenSizeInverse",
		1.0f / globalRendering->viewSizeX,
		1.0f / globalRendering->viewSizeY
	);
	const auto& identityMat = CMatrix44f::Identity();
	decalShader->SetUniformMatrix4x4("shadowMatrix", false, &identityMat.m[0]);

	decalShader->Disable();

	decalShader->Validate();
}

void CGroundDecalHandler::BindGroundAtlasTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(groundDecalAtlasMain->GetTexTarget(), groundDecalAtlasMain->GetTexID());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(groundDecalAtlasNorm->GetTexTarget(), groundDecalAtlasNorm->GetTexID());
}

void CGroundDecalHandler::BindAtlasTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(atlas->GetTexTarget(), atlas->GetTexID());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(atlas->GetTexTarget(), atlas->GetTexID());
}

void CGroundDecalHandler::BindCommonTextures()
{
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture());

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, heightMapTexture->GetTextureID());

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GetDepthBufferTextureTarget(), depthBufferCopy->GetDepthBufferTexture(highQuality));

	const CSMFReadMap* smfMap = smfDrawer->GetReadMap();
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, smfMap->GetNormalsTexture());

	if (shadowHandler.ShadowsLoaded()) {
		shadowHandler.SetupShadowTexSampler(GL_TEXTURE6, true);

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, shadowHandler.GetColorTextureID());
	}

	glActiveTexture(GL_TEXTURE0);
}

void CGroundDecalHandler::UnbindTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(atlas->GetTexTarget(), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(atlas->GetTexTarget(), 0);

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

	const float altitude = pos.y - CGround::GetHeightReal(pos.x, pos.z, false);
	const float3 groundNormal = CGround::GetNormal(pos.x, pos.z, false);

	// no decals for below-ground explosions
	if (altitude <= -1.0f)
		return;
	if (altitude >= radius)
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

	const int ttl = static_cast<int>(std::clamp(decalLevel * damage * 3.0f, 15.0f, decalLevel * 1800.0f));
	float alpha = std::clamp(2.0f * damage / 255.0f, 0.20f, 2.0f);
	float alphaDecay = alpha / ttl;
	float size = radius * math::SQRT2;

	const float2 posTL = { pos.x - size, pos.z - size };
	const float2 posTR = { pos.x + size, pos.z - size };
	const float2 posBR = { pos.x + size, pos.z + size };
	const float2 posBL = { pos.x - size, pos.z + size };

	const int scarIdx = 1 + guRNG.NextInt(maxUniqueScars); //not inclusive
	const auto mainName = IntToString(scarIdx, "mainscar_%i");
	const auto normName = IntToString(scarIdx, "normscar_%i");

	const auto createFrame = static_cast<uint32_t>(std::max(gs->frameNum, 0));

	const auto& decal = temporaryDecals.emplace_back(GroundDecal{
		.posTL = posTL,
		.posTR = posTR,
		.posBR = posBR,
		.posBL = posBL,
		.texMainOffsets = atlas->GetTextureWithBackup(mainName, "%FB_MAIN%"),
		.texNormOffsets = atlas->GetTextureWithBackup(normName, "%FB_NORM%"),
		.alpha = alpha,
		.alphaFalloff = alphaDecay,
		.rot = guRNG.NextFloat() * math::TWOPI,
		.height = size,
		.createFrameMin = createFrame,
		.createFrameMax = createFrame,
		.uvWrapDistance = 0.0f,
		.uvTraveledDistance = 0.0f,
		.id = GroundDecal::GetNextId()
	});

	decalIdToTmpDecalsVecPos[decal.id] = temporaryDecals.size() - 1;
	tmpUpdateIndicator.emplace_back(true);
	tmpNeedsUpdate = true;
}

void CGroundDecalHandler::ReloadTextures()
{
	atlas->ReloadTextures();
	{
		groundDecalAtlasMain = nullptr;
		groundDecalAtlasNorm = nullptr;
		groundDecalAtlasMain = std::make_unique<TextureRenderAtlas>(CTextureAtlas::ATLAS_ALLOC_QUADTREE, 0, 0, GL_RGBA8, "BuildingDecalsMain");
		groundDecalAtlasNorm = std::make_unique<TextureRenderAtlas>(CTextureAtlas::ATLAS_ALLOC_QUADTREE, 0, 0, GL_RGBA8, "BuildingDecalsNorm");
		AddBuildingDecalTextures();
	}
}

void CGroundDecalHandler::Draw()
{
	if (!GetDrawDecals())
		return;

	if (!smfDrawer->UseAdvShading())
		return;

	if (temporaryDecals.empty() && permanentDecals.empty())
		return;

	if (instTmpVBO.GetSize() < temporaryDecals.size() * sizeof(GroundDecal)) {
		vaoTmp.Bind();

		instTmpVBO.Bind();
		instTmpVBO.New(temporaryDecals.capacity() * sizeof(GroundDecal), GL_STREAM_DRAW);
		BindVertexAtrribs();

		vaoTmp.Unbind();

		UnbindVertexAtrribs();
		instTmpVBO.Unbind();
		std::fill(tmpUpdateIndicator.begin(), tmpUpdateIndicator.end(), true);
		tmpNeedsUpdate = true;
	}

	if (instPermVBO.GetSize() < permanentDecals.size() * sizeof(GroundDecal)) {
		vaoPerm.Bind();

		instPermVBO.Bind();
		instPermVBO.New(permanentDecals.capacity() * sizeof(GroundDecal), GL_STREAM_DRAW);
		BindVertexAtrribs();

		vaoPerm.Unbind();
		UnbindVertexAtrribs();
		instPermVBO.Unbind();
		permNeedsUpdate = true;
	}

	if (tmpNeedsUpdate) {
		instTmpVBO.Bind();
		const auto stt = tmpUpdateIndicator.begin();
		const auto fin = tmpUpdateIndicator.end();

		auto beg = tmpUpdateIndicator.begin();
		auto end = tmpUpdateIndicator.end();

		static const auto dirtyPred = [](bool d) -> bool { return d; };

		while (beg != fin) {
			beg = std::find_if    (beg, fin, dirtyPred);
			end = std::find_if_not(beg, fin, dirtyPred);
			if (beg != fin) {
				const uint32_t offs = static_cast<uint32_t>(std::distance(stt, beg));
				const uint32_t size = static_cast<uint32_t>(std::distance(beg, end));

				instTmpVBO.SetBufferSubData(offs * sizeof(GroundDecal), size * sizeof(GroundDecal), temporaryDecals.data() + offs/* in element */);

				std::fill(beg, end, false);
			}

			beg = end;
		}
		tmpNeedsUpdate = false;
		instTmpVBO.Unbind();
	}

	if (permNeedsUpdate) {
		instPermVBO.Bind();
		instPermVBO.SetBufferSubData(permanentDecals);
		instPermVBO.Unbind();
		permNeedsUpdate = false;
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

	decalShader->SetFlag("HAVE_SHADOWS", shadowHandler.ShadowsLoaded());
	decalShader->Enable();
	decalShader->SetUniform("curAdjustedFrame", std::max(gs->frameNum, 0) + globalRendering->timeOffset);
	if (shadowHandler.ShadowsLoaded())
		decalShader->SetUniformMatrix4x4("shadowMatrix", false, shadowHandler.GetShadowMatrixRaw());



	if (!permanentDecals.empty()) {
		BindGroundAtlasTextures();

		vaoPerm.Bind();
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, permanentDecals.size());
		vaoPerm.Unbind();
	}

	BindAtlasTextures();
	vaoTmp.Bind();
	glDrawArraysInstanced(GL_TRIANGLES, 0, 36, temporaryDecals.size());
	vaoTmp.Unbind();

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

	if (decalOwners.contains(object))
		return; // already added

	const auto createFrame = static_cast<uint32_t>(std::max(gs->frameNum, 0));

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

	const auto& decal = permanentDecals.emplace_back(GroundDecal{
		.posTL = posTL,
		.posTR = posTR,
		.posBR = posBR,
		.posBL = posBL,
		.texMainOffsets = groundDecalAtlasMain->GetTexture(                   (decalDef.groundDecalTypeName), "%FB_MAIN%"),
		.texNormOffsets = groundDecalAtlasNorm->GetTexture(GetExtraTextureName(decalDef.groundDecalTypeName), "%FB_NORM%"),
		.alpha = 1.0f,
		.alphaFalloff = 0.0f,
		.rot = 0.0f,
		.height = height,
		.createFrameMin = createFrame,
		.createFrameMax = createFrame,
		.uvWrapDistance = 0.0f,
		.uvTraveledDistance = 0.0f,
		.id = GroundDecal::GetNextId()
	});

	decalOwners[object] = decal.id;

	permNeedsUpdate = true;
}

void CGroundDecalHandler::RemoveSolidObject(const CSolidObject* object, const GhostSolidObject* gb)
{
	assert(object);

	const auto doIt = decalOwners.find(object);
	if (doIt == decalOwners.end()) {
		// it's ok for an object to not have any decals
		return;
	}

	const uint32_t decalID = doIt->second;

	if (decalID == 0) {
		LOG_L(L_ERROR, "[%s] Invalid zero decal id of object id = %u", __func__, object->id);
		return;
	}

	if (gb) {
		decalOwners[gb] = decalID;
		decalOwners.erase(object);
		return;
	}

	const auto pdIt = std::find_if(permanentDecals.begin(), permanentDecals.end(), [&decalID](const auto& permanentDecal) {
		return (permanentDecal.id == decalID);
	});

	if (pdIt != permanentDecals.end()) {
		const auto createFrame = static_cast<uint32_t>(std::max(gs->frameNum, 0));

		pdIt->alphaFalloff = object->GetDef()->decalDef.groundDecalDecaySpeed / GAME_SPEED;
		pdIt->createFrameMin = createFrame;
		pdIt->createFrameMax = createFrame;

		decalOwners.erase(object);
		permNeedsUpdate = true;
		return;
	}
	LOG_L(L_ERROR, "[%s] Invalid decal id = %u", __func__, decalID);
}

/**
 * @brief immediately remove an object's ground decal, if any (without fade out)
 */
void CGroundDecalHandler::ForceRemoveSolidObject(const CSolidObject* object)
{
	RemoveSolidObject(object, nullptr);
}

//void CGroundDecalHandler::UnitMoved(const CUnit* unit) { /*AddDecal(unit, unit->pos);*/ }

void CGroundDecalHandler::GhostDestroyed(const GhostSolidObject* gb) {
	/*
	if (gb->decal == nullptr)
		return;

	gb->decal->gbOwner = nullptr;

	//If a ghost wasn't drawn, remove the decal
	if (gb->lastDrawFrame < (globalRendering->drawFrame - 1))
		gb->decal->alpha = 0.0f;
	*/
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

void CGroundDecalHandler::AddTrack(const CUnit* unit, const float3& newPos)
{
	if (!GetDrawDecals())
		return;

	const UnitDef* unitDef = unit->unitDef;
	const SolidObjectDecalDef& decalDef = unitDef->decalDef;

	const float3 decalPos = newPos + unit->frontdir * decalDef.trackDecalOffset;

	if (!unit->leaveTracks)
		return;

	if (!unitDef->IsGroundUnit())
		return;

	const float trackLifeTime = decalLevel * GAME_SPEED * decalDef.trackDecalStrength;
	if (trackLifeTime <= 0.0f)
		return;

	auto& mm = unitMinMaxHeights[unit->id];

	if (!CanReceiveTracks(decalPos) || (unit->IsInWater() && !unit->IsOnGround())) {
		decalOwners.erase(unit); // restart with new decal next time
		mm = MINMAX_HEIGHT_INIT;
		return;
	}

	const float2 decalPos2 = float2(decalPos.x, decalPos.z);
	const float2 wc = float2(
		unit->rightdir.x * decalDef.trackDecalWidth * 0.5f,
		unit->rightdir.z * decalDef.trackDecalWidth * 0.5f
	);

	const auto createFrame = static_cast<uint32_t>(std::max(gs->frameNum, 0));
	const uint32_t decalID = decalOwners[unit];
	if (decalID == 0) {
		// new decal

		const auto& mainName = decalDef.trackDecalTypeName;
		const auto  normName = GetExtraTextureName(mainName);

		const float alphaDecay = 1.0f / trackLifeTime;

		const auto& newDecal = temporaryDecals.emplace_back(GroundDecal{
			.posTL = decalPos2 - wc,
			.posTR = decalPos2 - wc,
			.posBR = decalPos2 + wc,
			.posBL = decalPos2 + wc,
			.texMainOffsets = atlas->GetTextureWithBackup(mainName, "%FB_MAIN%"),
			.texNormOffsets = atlas->GetTextureWithBackup(normName, "%FB_NORM%"),
			.alpha = 1.0f,
			.alphaFalloff = alphaDecay,
			.rot = 0.0f,
			.height = 0.0f,
			.createFrameMin = createFrame,
			.createFrameMax = createFrame,
			.uvWrapDistance = decalDef.trackDecalWidth * decalDef.trackDecalStretch,
			.uvTraveledDistance = 0.0f,
			.id = GroundDecal::GetNextId()
		});

		decalOwners[unit] = newDecal.id;
		mm = MINMAX_HEIGHT_INIT;

		decalIdToTmpDecalsVecPos[newDecal.id] = temporaryDecals.size() - 1;
		tmpUpdateIndicator.emplace_back(true);
		tmpNeedsUpdate = true;
		return;
	}

	float decalHeight = CGround::GetHeightReal(decalPos.x, decalPos.z, false);
	mm.first  = std::min(mm.first , decalHeight);
	mm.second = std::max(mm.second, decalHeight);

	if (createFrame % TRACKS_UPDATE_RATE != 0)
		return;

	const size_t vecPos = decalIdToTmpDecalsVecPos[decalID];
	assert(vecPos < temporaryDecals.size());
	GroundDecal& oldDecal = temporaryDecals[vecPos];

	// check if the unit is standing still
	if (oldDecal.createFrameMax + TRACKS_UPDATE_RATE < createFrame) {
		decalOwners.erase(unit);
		mm = MINMAX_HEIGHT_INIT;
		return;
	}

	const float2 posL = (oldDecal.posTL + oldDecal.posBL) * 0.5f;
	const float2 posR = (oldDecal.posTR + oldDecal.posBR) * 0.5f;

	const float2 dirO = (posR      - posL).SafeNormalize();
	const float2 dirN = (decalPos2 - posR).SafeNormalize();

	if (dirO.Dot(dirN) >= 0.9999f) {
		oldDecal.posTR = decalPos2 - wc;
		oldDecal.posBR = decalPos2 + wc;
		oldDecal.createFrameMax = createFrame;

		const float2 midPointDist = (oldDecal.posTL + oldDecal.posTR + oldDecal.posBR + oldDecal.posBL) * 0.25f;
		const float midPointHeight = CGround::GetHeightReal(midPointDist.x, midPointDist.y, false);
		oldDecal.height = std::max(mm.second - midPointHeight, midPointHeight - mm.first) + 1.0f;

		tmpUpdateIndicator[vecPos] = true;
		tmpNeedsUpdate = true;
		return;
	}

	// new decal, starting where the previous ended
	auto& newDecal = temporaryDecals.emplace_back(GroundDecal{
		.posTL = oldDecal.posTR,
		.posTR = decalPos2 - wc,
		.posBR = decalPos2 + wc,
		.posBL = oldDecal.posBR,
		.texMainOffsets = oldDecal.texMainOffsets,
		.texNormOffsets = oldDecal.texNormOffsets,
		.alpha = oldDecal.alpha,
		.alphaFalloff = oldDecal.alphaFalloff,
		.rot = 0.0f,
		.height = 0.0f, //set later
		.createFrameMin = oldDecal.createFrameMax,
		.createFrameMax = createFrame,
		.uvWrapDistance = decalDef.trackDecalWidth * decalDef.trackDecalStretch,
		.uvTraveledDistance = posL.Distance(posR),
		.id = GroundDecal::GetNextId()
	});

	const float2 midPointDist = (newDecal.posTL + newDecal.posTR + newDecal.posBR + newDecal.posBL) * 0.25f;
	const float midPointHeight = CGround::GetHeightReal(midPointDist.x, midPointDist.y, false);
	newDecal.height = std::max(mm.second - midPointHeight, midPointHeight - mm.first) + 1.0f;

	decalOwners[unit] = newDecal.id;

	decalIdToTmpDecalsVecPos[newDecal.id] = temporaryDecals.size() - 1;
	tmpUpdateIndicator.emplace_back(true);
	tmpNeedsUpdate = true;
}

void CGroundDecalHandler::UpdateTemporaryDecalsVector(int frameNum)
{
	if (temporaryDecals.empty())
		return;

	// only bother with the following code, if number of items is big enough
	if (temporaryDecals.size() < temporaryDecals.capacity() >> 6)
		return;

	size_t numToDelete = 0;
	for (auto& decal : temporaryDecals) {
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

	if (static_cast<float>(temporaryDecals.size()) / static_cast<float>(numToDelete) > RESORT_THRESHOLD) {
		// group all expired items towards the end of the vector
		auto partIt = std::stable_partition(temporaryDecals.begin(), temporaryDecals.end(), [](const auto& item) {
			return item.IsValid();
			});
		// erase irrelevant items
		for (auto it = partIt; it < temporaryDecals.end(); ++it) {
			decalIdToTmpDecalsVecPos.erase(it->id);
		}
		// remove expired decals
		temporaryDecals.resize(temporaryDecals.size() - numToDelete);
		// update relevant items
		for (size_t i = 0; i < temporaryDecals.size(); ++i) {
			decalIdToTmpDecalsVecPos[temporaryDecals[i].id] = i;
		}

		tmpUpdateIndicator.resize(temporaryDecals.size());
		std::fill(tmpUpdateIndicator.begin(), tmpUpdateIndicator.end(), true);
		tmpNeedsUpdate = true;
	}
};

void CGroundDecalHandler::GameFrame(int frameNum)
{
	/*
	const auto UpdateDecalsVector = [frameNum, this](std::vector<GroundDecal>& groundDecalsVec) -> bool {
		if (groundDecalsVec.empty())
			return false;

		// only bother with the following code, if number of items is big enough
		if (groundDecalsVec.size() < groundDecalsVec.capacity() >> 6)
			return false;

		size_t numToDelete = 0;
		for (auto& decal : groundDecalsVec) {
			if (!decal.IsValid()) {
				numToDelete++;
				continue;
			}
			if (frameNum - decal.createFrame > static_cast<int>(decal.alpha / decal.alphaFalloff)) {
				decal.MarkInvalid();
				numToDelete++;
			}
		}

		if (numToDelete == 0)
			return false;

		// resort if number of expired items > 25.0%
		static constexpr float RESORT_THRESHOLD = 1.0f / 4.0f;

		if (static_cast<float>(groundDecalsVec.size()) / static_cast<float>(numToDelete) > RESORT_THRESHOLD) {
			// group all expired items towards the end of the vector
			auto partIt = std::stable_partition(groundDecalsVec.begin(), groundDecalsVec.end(), [](const auto& item) {
				return item.IsValid();
			});
			// erase irrelevant items
			for (auto it = partIt; it < groundDecalsVec.end(); ++it) {
				decalIdToTmpDecalsVecPos.erase(it->id);
			}
			// remove expired decals
			groundDecalsVec.resize(groundDecalsVec.size() - numToDelete);
			// update relevant items
			for (size_t i = 0; i < groundDecalsVec.size(); ++i) {
				decalIdToTmpDecalsVecPos[groundDecalsVec[i].id] = i;
			}

			return true;
		}
		return false;
	};
	*/

	if (frameNum % 16 ==  0) {
		UpdateTemporaryDecalsVector(frameNum);
	}
	if (frameNum % 16 == 15) {
		// FIXME
		//UpdateDecalsVector(permanentDecals);
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
//void CGroundDecalHandler::FeatureMoved(const CFeature* feature, const float3& oldpos) { AddSolidObject(feature); }

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

void CGroundDecalHandler::RenderUnitCreated(const CUnit* unit, int cloaked) {
	AddSolidObject(unit);
}
void CGroundDecalHandler::RenderUnitDestroyed(const CUnit* unit) {
	RemoveSolidObject(unit, nullptr);
	decalOwners.erase(unit);
}

void CGroundDecalHandler::RenderFeatureCreated(const CFeature* feature) { AddSolidObject(feature); }
void CGroundDecalHandler::RenderFeatureDestroyed(const CFeature* feature) { RemoveSolidObject(feature, nullptr); }

// FIXME: Add a RenderUnitLoaded event
void CGroundDecalHandler::UnitLoaded(const CUnit* unit, const CUnit* transport) { ForceRemoveSolidObject(unit); }
void CGroundDecalHandler::UnitUnloaded(const CUnit* unit, const CUnit* transport) { AddSolidObject(unit); }

void CGroundDecalHandler::UnitMoved(const CUnit* unit) { AddTrack(unit, unit->pos); }