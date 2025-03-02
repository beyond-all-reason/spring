/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cstring> // mem{set,cpy}

#include "xsimd/xsimd.hpp"
#include "SMFReadMap.h"
#include "SMFGroundTextures.h"
#include "SMFGroundDrawer.h"
#include "SMFFormat.h"
#include "Map/MapInfo.h"
#include "Map/Ground.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/LoadScreen.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Env/WaterRendering.h"
#include "Rendering/Env/SunLighting.h"
#include "Rendering/Env/ISky.h"
#include "Rendering/Env/SkyLight.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/PBO.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/GL/SubState.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Map/InfoTexture/IInfoTextureHandler.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Threading/ThreadPool.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringHash.h"
#include "System/LoadLock.h"
#include "System/XSimdOps.hpp"

#include "System/Misc/TracyDefs.h"

using std::max;

CONFIG(bool, GroundNormalTextureHighPrecision).deprecated(true);
CONFIG(float, SMFTexAniso).defaultValue(4.0f).minimumValue(0.0f);
CONFIG(float, SSMFTexAniso).defaultValue(4.0f).minimumValue(0.0f);



CSMFMapFile CSMFReadMap::mapFile;

std::vector<float> CSMFReadMap::cornerHeightMapSynced;
std::vector<float> CSMFReadMap::cornerHeightMapUnsynced;

static std::vector<float> normalPixels;

CSMFReadMap::CSMFReadMap(const std::string& mapName): CEventClient("[CSMFReadMap]", 271950, false)
{
	RECOIL_DETAILED_TRACY_ZONE;
	loadscreen->SetLoadMessage("Loading SMF");
	eventHandler.AddClient(this);

	//auto lock = CLoadLock::GetUniqueLock();

	mapFile.Close();
	mapFile.Open(mapName);

	haveSpecularTexture = !(mapInfo->smf.specularTexName.empty());
	haveSplatDetailDistribTexture = (!mapInfo->smf.splatDetailTexName.empty() && !mapInfo->smf.splatDistrTexName.empty());
	haveSplatNormalDistribTexture = false;

	for (const MapTexture& mapTex: splatNormalTextures) {
		assert(!mapTex.HasLuaTex());
		assert(mapTex.GetID() == 0);
	}

	for (const std::string& texName: mapInfo->smf.splatDetailNormalTexNames) {
		haveSplatNormalDistribTexture |= !texName.empty();
	}

	// Detail Normal Splatting requires at least one splatDetailNormalTexture and a distribution texture
	haveSplatNormalDistribTexture &= !mapInfo->smf.splatDistrTexName.empty();

	ParseHeader();
	LoadHeightMap();
	CReadMap::Initialize();

	ConfigureTexAnisotropyLevels();
	{
		auto lock = CLoadLock::GetUniqueLock();

		LoadMinimap();

		CreateSpecularTex();
		CreateSplatDetailTextures();
		CreateGrassTex();
		CreateDetailTex();
		CreateShadingTex();
		CreateNormalTex();
		CreateHeightMapTex();
		CreateShadingGL();
	}

	mapFile.ReadFeatureInfo();
}

CSMFReadMap::~CSMFReadMap()
{
	shadingFBO = nullptr;
	shaderHandler->ReleaseProgramObject("[CSMFReadMap]", "ShadingShader");
	mapFile.Close();
}



void CSMFReadMap::ParseHeader()
{
	const SMFHeader& header = mapFile.GetHeader();

	mapDims.mapx = header.mapx;
	mapDims.mapy = header.mapy;

	numBigTexX      = (header.mapx / bigSquareSize);
	numBigTexY      = (header.mapy / bigSquareSize);
	bigTexSize      = (SQUARE_SIZE * bigSquareSize);
	tileMapSizeX    = (header.mapx / tileScale);
	tileMapSizeY    = (header.mapy / tileScale);
	tileCount       = (header.mapx * header.mapy) / (tileScale * tileScale);
	mapSizeX        = (header.mapx * SQUARE_SIZE);
	mapSizeZ        = (header.mapy * SQUARE_SIZE);
	maxHeightMapIdx = ((header.mapx + 1) * (header.mapy + 1)) - 1;
	heightMapSizeX  =  (header.mapx + 1);
}


void CSMFReadMap::LoadHeightMap()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const SMFHeader& header = mapFile.GetHeader();

	cornerHeightMapSynced.clear();
	cornerHeightMapSynced.resize((mapDims.mapx + 1) * (mapDims.mapy + 1)); //mapDims.mapxp1, mapDims.mapyp1 are not available here
	cornerHeightMapUnsynced.clear();
	cornerHeightMapUnsynced.resize((mapDims.mapx + 1) * (mapDims.mapy + 1));

	heightMapSyncedPtr   = &cornerHeightMapSynced;
	heightMapUnsyncedPtr = &cornerHeightMapUnsynced;

	const float minHgt = mapInfo->smf.minHeightOverride ? mapInfo->smf.minHeight : header.minHeight;
	const float maxHgt = mapInfo->smf.maxHeightOverride ? mapInfo->smf.maxHeight : header.maxHeight;

	float* cornerHeightMapSyncedData = cornerHeightMapSynced.data();
	float* cornerHeightMapUnsyncedData = cornerHeightMapUnsynced.data();

	// FIXME:
	//     callchain CReadMap::Initialize --> CReadMap::UpdateHeightMapSynced(0, 0, mapDims.mapx, mapDims.mapy) -->
	//     PushVisibleHeightMapUpdate --> (next UpdateDraw) UpdateHeightMapUnsynced(0, 0, mapDims.mapx, mapDims.mapy)
	//     initializes the UHM a second time
	//     merge them some way so UHM & shadingtex is available from the time readMap got created
	mapFile.ReadHeightmap(cornerHeightMapSyncedData, cornerHeightMapUnsyncedData, minHgt, (maxHgt - minHgt) / 65536.0f);
}


void CSMFReadMap::LoadMinimap()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CBitmap minimapTexBM;

	if (minimapTexBM.Load(mapInfo->smf.minimapTexName)) {
		minimapTex.SetRawTexID(minimapTexBM.CreateTexture());
		minimapTex.SetRawSize(int2(minimapTexBM.xsize, minimapTexBM.ysize));
		return;
	}

	// the minimap is a static texture
	std::vector<unsigned char> minimapTexBuf(MINIMAP_SIZE, 0);
	mapFile.ReadMinimap(&minimapTexBuf[0]);
	// default; only valid for mip 0
	minimapTex.SetRawSize(int2(1024, 1024));

	glGenTextures(1, minimapTex.GetIDPtr());
	glBindTexture(GL_TEXTURE_2D, minimapTex.GetID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MINIMAP_NUM_MIPMAP - 1);
	int offset = 0;
	for (unsigned int i = 0; i < MINIMAP_NUM_MIPMAP; i++) {
		const int mipsize = 1024 >> i;
		const int size = ((mipsize + 3) / 4) * ((mipsize + 3) / 4) * 8;
		glCompressedTexImage2DARB(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mipsize, mipsize, 0, size, &minimapTexBuf[0] + offset);
		offset += size;
	}
}

void CSMFReadMap::CreateSpecularTex()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!haveSpecularTexture)
		return;

	{
		CBitmap specularTexBM;

		// maps wants specular lighting, but no moderation
		if (!specularTexBM.Load(mapInfo->smf.specularTexName)) {
			LOG_L(L_WARNING, "[CSMFReadMap::%s] Invalid SMF specularTex %s. Creating fallback texture", __func__, mapInfo->smf.specularTexName.c_str());
			specularTexBM.AllocDummy(SColor(255, 255, 255, 255));
		}

		specularTex.SetRawTexID(specularTexBM.CreateTexture());
		specularTex.SetRawSize(int2(specularTexBM.xsize, specularTexBM.ysize));
	}

	{
		CBitmap skyReflectModTexBM;

		// no default 1x1 textures for these
		if (skyReflectModTexBM.Load(mapInfo->smf.skyReflectModTexName)) {
			skyReflectModTex.SetRawTexID(skyReflectModTexBM.CreateTexture());
			skyReflectModTex.SetRawSize(int2(skyReflectModTexBM.xsize, skyReflectModTexBM.ysize));
		}
	}

	{
		CBitmap blendNormalsTexBM;

		if (blendNormalsTexBM.Load(mapInfo->smf.blendNormalsTexName)) {
			blendNormalsTex.SetRawTexID(blendNormalsTexBM.CreateTexture());
			blendNormalsTex.SetRawSize(int2(blendNormalsTexBM.xsize, blendNormalsTexBM.ysize));
		}
	}

	{
		CBitmap lightEmissionTexBM;

		if (lightEmissionTexBM.Load(mapInfo->smf.lightEmissionTexName)) {
			lightEmissionTex.SetRawTexID(lightEmissionTexBM.CreateTexture());
			lightEmissionTex.SetRawSize(int2(lightEmissionTexBM.xsize, lightEmissionTexBM.ysize));
		}
	}

	{
		CBitmap parallaxHeightTexBM;

		if (parallaxHeightTexBM.Load(mapInfo->smf.parallaxHeightTexName)) {
			parallaxHeightTex.SetRawTexID(parallaxHeightTexBM.CreateTexture());
			parallaxHeightTex.SetRawSize(int2(parallaxHeightTexBM.xsize, parallaxHeightTexBM.ysize));
		}
	}
}

void CSMFReadMap::CreateSplatDetailTextures()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!haveSplatDetailDistribTexture)
		return;

	{
		CBitmap splatDetailTexBM;

		// if a map supplies an intensity- AND a distribution-texture for
		// detail-splat blending, the regular detail-texture is not used
		// default detail-texture should be all-grey
		if (!splatDetailTexBM.Load(mapInfo->smf.splatDetailTexName)) {
			LOG_L(L_WARNING, "[CSMFReadMap::%s] Invalid SMF splatDetailTex %s. Creating fallback texture", __func__, mapInfo->smf.splatDetailTexName.c_str());
			splatDetailTexBM.AllocDummy(SColor(127, 127, 127, 127));
		}

		splatDetailTex.SetRawTexID(splatDetailTexBM.CreateMipMapTexture(texAnisotropyLevels[true], 0.0f, 0));
		splatDetailTex.SetRawSize(int2(splatDetailTexBM.xsize, splatDetailTexBM.ysize));
	}

	{
		CBitmap splatDistrTexBM;

		if (!splatDistrTexBM.Load(mapInfo->smf.splatDistrTexName)) {
			LOG_L(L_WARNING, "[CSMFReadMap::%s] Invalid SMF splatDistrTex %s. Creating fallback texture", __func__, mapInfo->smf.splatDistrTexName.c_str());
			splatDistrTexBM.AllocDummy(SColor(255, 0, 0, 0));
		}

		splatDistrTex.SetRawTexID(splatDistrTexBM.CreateMipMapTexture(texAnisotropyLevels[true], 0.0f, 0));
		splatDistrTex.SetRawSize(int2(splatDistrTexBM.xsize, splatDistrTexBM.ysize));
	}

	// only load the splat detail normals if any of them are defined and present
	if (!haveSplatNormalDistribTexture)
		return;

	for (size_t i = 0; i < mapInfo->smf.splatDetailNormalTexNames.size(); i++) {
		if (i == NUM_SPLAT_DETAIL_NORMALS)
			break;

		CBitmap splatDetailNormalTextureBM;

		if (!splatDetailNormalTextureBM.Load(mapInfo->smf.splatDetailNormalTexNames[i])) {
			splatDetailNormalTextureBM.Alloc(1, 1, 4);
			splatDetailNormalTextureBM.GetRawMem()[0] = 127; // RGB is packed standard normal map
			splatDetailNormalTextureBM.GetRawMem()[1] = 127;
			splatDetailNormalTextureBM.GetRawMem()[2] = 255; // With a single upward (+Z) pointing vector
			splatDetailNormalTextureBM.GetRawMem()[3] = 127; // Alpha is diffuse as in old-style detail textures
		}

		splatNormalTextures[i].SetRawTexID(splatDetailNormalTextureBM.CreateMipMapTexture(texAnisotropyLevels[true], 0.0f, 0));
		splatNormalTextures[i].SetRawSize(int2(splatDetailNormalTextureBM.xsize, splatDetailNormalTextureBM.ysize));
	}

}


void CSMFReadMap::CreateGrassTex()
{
	RECOIL_DETAILED_TRACY_ZONE;
	grassShadingTex.SetRawTexID(minimapTex.GetID());
	grassShadingTex.SetRawSize(int2(1024, 1024));

	CBitmap grassShadingTexBM;

	if (!grassShadingTexBM.Load(mapInfo->smf.grassShadingTexName))
		return;

	// override minimap
	grassShadingTex.SetRawTexID(grassShadingTexBM.CreateMipMapTexture());
	grassShadingTex.SetRawSize(int2(grassShadingTexBM.xsize, grassShadingTexBM.ysize));
}


void CSMFReadMap::CreateDetailTex()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CBitmap detailTexBM;

	if (!detailTexBM.Load(mapInfo->smf.detailTexName)) {
		LOG_L(L_WARNING, "[CSMFReadMap::%s] Invalid SMF detailTex %s. Creating fallback texture", __func__, mapInfo->smf.detailTexName.c_str());
		detailTexBM.AllocDummy({127, 127, 127, 0});
	}

	detailTex.SetRawTexID(detailTexBM.CreateMipMapTexture(texAnisotropyLevels[false], 0.0f, 0));
	detailTex.SetRawSize(int2(detailTexBM.xsize, detailTexBM.ysize));
}


void CSMFReadMap::CreateShadingTex()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// +1 to accomodate two FBO attachments of same size, not fully correct
	shadingTex.SetRawSize(int2(mapDims.mapxp1, mapDims.mapyp1));

	// the shading/normal texture buffers must have PO2 dimensions
	// (excess elements that no vertices map into are left unused)
	glGenTextures(1, shadingTex.GetIDPtr());
	glBindTexture(GL_TEXTURE_2D, shadingTex.GetID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	if (texAnisotropyLevels[false] != 0.0f)
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, texAnisotropyLevels[false]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, shadingTex.GetSize().x, shadingTex.GetSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}


void CSMFReadMap::CreateNormalTex()
{
	RECOIL_DETAILED_TRACY_ZONE;
	normalsTex.SetRawSize(int2(mapDims.mapxp1, mapDims.mapyp1));

	glGenTextures(1, normalsTex.GetIDPtr());
	glBindTexture(GL_TEXTURE_2D, normalsTex.GetID());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	constexpr GLint swizzleMask[] = { GL_RED, GL_GREEN, GL_GREEN, GL_GREEN };
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, (normalsTex.GetSize()).x, (normalsTex.GetSize()).y, 0, GL_RG, GL_FLOAT, nullptr);
}

void CSMFReadMap::CreateHeightMapTex()
{
	glGenTextures(1, heightMapTexture.GetIDPtr());
	glBindTexture(GL_TEXTURE_2D, heightMapTexture.GetID());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	constexpr GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F,
		mapDims.mapxp1, mapDims.mapyp1, 0,
		GL_RED, GL_FLOAT, nullptr
	);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void CSMFReadMap::CreateShadingGL()
{
	shadingFBO = std::make_unique<FBO>(false);

	shadingFBO->Bind();
	shadingFBO->AttachTexture(shadingTex.GetID(), GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0, 0);
	shadingFBO->AttachTexture(normalsTex.GetID(), GL_TEXTURE_2D, GL_COLOR_ATTACHMENT1, 0);
	constexpr GLenum DRAW_BUFFERS[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, DRAW_BUFFERS);
	shadingFBO->CheckStatus("SMF-SHADING");
	shadingFBO->Unbind();

	shadingShader = shaderHandler->CreateProgramObject("[CSMFReadMap]", "ShadingShader");
	shadingShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/SMFShadingTextureVertProg.glsl", "", GL_VERTEX_SHADER));
	shadingShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/SMFShadingTextureFragProg.glsl", "", GL_FRAGMENT_SHADER));
	shadingShader->BindAttribLocations<VA_TYPE_2D0>();
	shadingShader->BindOutputLocation("shadingVal", 0);
	shadingShader->BindOutputLocation("normalXZ", 1);
	shadingShader->Link();

	shadingShader->Enable();
	shadingShader->SetUniform("mapSizeP1",
		static_cast<float>(mapDims.mapxp1), static_cast<float>(mapDims.mapyp1),
		           1.0f / (mapDims.mapxp1),            1.0f / (mapDims.mapyp1)
	);

	shadingShader->SetUniform("heightMapTex", 0);
	shadingShader->SetUniform4v("groundAmbientColor", &sunLighting->groundAmbientColor.x);
	shadingShader->SetUniform4v("groundDiffuseColor", &sunLighting->groundDiffuseColor.x);
	shadingShader->SetUniform("lightDir", 0.0f, 0.0f, 0.0f, 0.0f); // envParams.sun.dir is not yet available
	shadingShader->SetUniform3v("waterBaseColor", &waterRendering->baseColor.x);
	shadingShader->SetUniform3v("waterAbsorb", &waterRendering->absorb.x);
	shadingShader->SetUniform3v("waterMinColor", &waterRendering->minColor.x);
	shadingShader->SetUniform("waterLevel", CGround::GetWaterPlaneLevel());
	shadingShader->Disable();

	shadingShader->Validate();
}

void CSMFReadMap::UpdateHeightMapUnsynced(const SRectangle& update)
{
	RECOIL_DETAILED_TRACY_ZONE;
	UpdateCornerHeightMapUnsynced(update);
	UpdateHeightMapTexture(update);
	UpdateHeightBoundsUnsynced(update);
	UpdateFaceNormalsUnsynced(update);
	UpdateVisNormalsAndShadingTexture(update);
}

void CSMFReadMap::UpdateHeightMapUnsyncedPost()
{
	RECOIL_DETAILED_TRACY_ZONE;
	static_assert(bigSquareSize == PATCH_SIZE, "");

	for (uint32_t pz = 0; pz < numBigTexY; ++pz) {
		for (uint32_t px = 0; px < numBigTexX; ++px) {
			if (unsyncedHeightInfo[pz * numBigTexX + px].x != std::numeric_limits<float>::max())
				continue;

			for (uint32_t vz = 0; vz <= bigSquareSize; ++vz) {
				const size_t idx0 = (pz * bigSquareSize + vz) * mapDims.mapxp1 + px * bigSquareSize;
				const size_t idx1 = idx0 + bigSquareSize + 1;

				unsyncedHeightInfo[pz * numBigTexX + px].arr = xsimd::reduce(
					cornerHeightMapUnsynced.data() + idx0,
					cornerHeightMapUnsynced.data() + idx1,
					unsyncedHeightInfo[pz * numBigTexX + px].arr,
					MinOp{}, MaxOp{}, PlusOp{}
				);
			}
			unsyncedHeightInfo[pz * numBigTexX + px].z /= Square(bigSquareSize + 1);
		}
	}
}

void CSMFReadMap::UpdateCornerHeightMapUnsynced(const SRectangle& update)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//corner space, inclusive
	for (int z = update.z1; z <= update.z2; z++) {
		{
			const int idx0 = (z * mapDims.mapxp1 + (update.x1));
			const int idx1 = (z * mapDims.mapxp1 + (update.x2 + 1));
			std::copy(
				cornerHeightMapSynced.begin() + idx0,
				cornerHeightMapSynced.begin() + idx1,
				cornerHeightMapUnsynced.begin() + idx0
			);
		}
	}
}

void CSMFReadMap::UpdateHeightMapTexture(const SRectangle& update)
{
	// consider full update if the area of update is >= 50% of full update
	const auto refFullUpdateThreshold = (mapDims.mapx * mapDims.mapy) >> 1;
	if (update.GetArea() >= refFullUpdateThreshold) {
		glBindTexture(GL_TEXTURE_2D, heightMapTexture.GetID());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mapDims.mapxp1, mapDims.mapyp1, GL_RED, GL_FLOAT, GetCornerHeightMapUnsynced());
		glBindTexture(GL_TEXTURE_2D, 0);

		return;
	}

	// partial update
	const int sizeX = update.GetWidth() + 1;
	const int sizeZ = update.GetHeight() + 1;

	PBO pbo;
	pbo.Bind();
	pbo.New(sizeX * sizeZ * sizeof(float));

	const float* heightMap = readMap->GetCornerHeightMapUnsynced();
	float* heightBuf = reinterpret_cast<float*>(pbo.MapBuffer(0, pbo.GetSize(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | pbo.mapUnsyncedBit));

	if (heightBuf != nullptr) {
		for (int z = 0; z < sizeZ; z++) {
			const auto* src = heightMap + update.x1 + (z + update.z1) * mapDims.mapxp1;
			      auto* dst = heightBuf +             (z            ) * sizeX;

			std::copy(src, src + sizeX, dst);
		}
	}

	pbo.UnmapBuffer();

	glBindTexture(GL_TEXTURE_2D, heightMapTexture.GetID());
	glTexSubImage2D(GL_TEXTURE_2D, 0, update.x1, update.z1, sizeX, sizeZ, GL_RED, GL_FLOAT, pbo.GetPtr());

	pbo.Invalidate();
	pbo.Unbind();

	glBindTexture(GL_TEXTURE_2D, 0);
}


void CSMFReadMap::UpdateHeightBoundsUnsynced(const SRectangle& update)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const uint32_t minPatchX = std::max(update.x1 / bigSquareSize, (0             ));
	const uint32_t minPatchZ = std::max(update.z1 / bigSquareSize, (0             ));
	const uint32_t maxPatchX = std::min(update.x2 / bigSquareSize, (numBigTexX - 1));
	const uint32_t maxPatchZ = std::min(update.z2 / bigSquareSize, (numBigTexY - 1));

	for (uint32_t pz = minPatchZ; pz <= maxPatchZ; ++pz) {
		for (uint32_t px = minPatchX; px <= maxPatchX; ++px) {
			unsyncedHeightInfo[pz * numBigTexX + px] = {
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::lowest(),
				0.0f
			};
		}
	}
}


void CSMFReadMap::UpdateFaceNormalsUnsynced(const SRectangle& update)
{
	RECOIL_DETAILED_TRACY_ZONE;

	const auto& sfn = faceNormalsSynced;
	      auto& ufn = faceNormalsUnsynced;
	const auto& scn = centerNormalsSynced;
	      auto& ucn = centerNormalsUnsynced;

	const float* heightmapUnsynced = GetCornerHeightMapUnsynced();

	// update is in corner space. Thus update x2/z2 - 1
	for (int z = update.z1; z < update.z2; z++) {
		{
			const int idx0 = (z * mapDims.mapx + update.x1    ) * 2;
			const int idx1 = (z * mapDims.mapx + update.x2 + 0) * 2;
			std::copy(
				sfn.begin() + idx0,
				sfn.begin() + idx1,
				ufn.begin() + idx0
			);
		}
		{
			const int idx0 = (z * mapDims.mapx + update.x1    );
			const int idx1 = (z * mapDims.mapx + update.x2 + 0);
			std::copy(
				scn.begin() + idx0,
				scn.begin() + idx1,
				ucn.begin() + idx0
			);
		}
	}

	// a heightmap update over (x1, y1) - (x2, y2) implies the
	// normals change over (x1 - 1, y1 - 1) - (x2 + 1, y2 + 1)

	const int minx = std::max(update.x1 - 1,              0);
	const int minz = std::max(update.z1 - 1,              0);
	const int maxx = std::min(update.x2 + 1, mapDims.mapxm1);
	const int maxz = std::min(update.z2 + 1, mapDims.mapym1);

	const auto EdgeNormalsUpdateBody = [&ufn, &ucn](int x, int z) {
		const int idxTL = (z + 0) * mapDims.mapxp1 + x; // TL
		const int idxBL = (z + 1) * mapDims.mapxp1 + x; // BL

		const float& hTL = cornerHeightMapUnsynced[idxTL + 0];
		const float& hTR = cornerHeightMapUnsynced[idxTL + 1];
		const float& hBL = cornerHeightMapUnsynced[idxBL + 0];
		const float& hBR = cornerHeightMapUnsynced[idxBL + 1];

		// normal of top-left triangle (face) in square
		//
		//  *---> e1
		//  |
		//  |
		//  v
		//  e2
		//const float3 e1( SQUARE_SIZE, hTR - hTL,           0);
		//const float3 e2(           0, hBL - hTL, SQUARE_SIZE);
		//const float3 fnTL = (e2.cross(e1)).Normalize();
		const float3 fnTL = float3{
			-(hTR - hTL),
			SQUARE_SIZE,
			-(hBL - hTL)
		}.Normalize();

		// normal of bottom-right triangle (face) in square
		//
		//         e3
		//         ^
		//         |
		//         |
		//  e4 <---*
		//const float3 e3(-SQUARE_SIZE, hBL - hBR,           0);
		//const float3 e4(           0, hTR - hBR,-SQUARE_SIZE);
		//const float3 fnBR = (e4.cross(e3)).Normalize();
		const float3 fnBR = float3{
			+(hBL - hBR),
			SQUARE_SIZE,
			+(hTR - hBR)
		}.Normalize();


		ufn[(z * mapDims.mapx + x) * 2 + 0] = fnTL;
		ufn[(z * mapDims.mapx + x) * 2 + 1] = fnBR;
		ucn[(z * mapDims.mapx + x)] = (fnTL + fnBR).Normalize();
	};

	//edges of the update rectangle need normals recalculation
	// zmin
	if (minz < update.z1) {
		for (int x = minx; x < maxx; ++x) {
			EdgeNormalsUpdateBody(x, minz);
		}
	}
	// zmax
	if (update.z2 < maxz) {
		for (int x = minx; x < maxx; ++x) {
			EdgeNormalsUpdateBody(x, update.z2);
		}
	}
	// xmin
	if (minx < update.x1) {
		for (int z = minz + 1; z < maxz - 1; ++z) {
			EdgeNormalsUpdateBody(minx, z);
		}
	}
	// xmax
	if (update.x2 < maxx) {
		for (int z = minz + 1; z < maxz - 1; ++z) {
			EdgeNormalsUpdateBody(update.x2, z);
		}
	}
}

void CSMFReadMap::UpdateShadingTexture()
{
	SRectangle update { 0, 0, mapDims.mapx, mapDims.mapy };
	UpdateVisNormalsAndShadingTexture(update);
}

void CSMFReadMap::UpdateVisNormalsAndShadingTexture(const SRectangle& update)
{
	RECOIL_DETAILED_TRACY_ZONE;

	assert(shadingFBO->IsValid() && shadingShader->IsValid());

	using namespace GL::State;
	auto state = GL::SubState(
		DepthTest(GL_FALSE),
		Blending(GL_FALSE)
	);

	// enlarge rect by 1pixel in all directions (cause we use center normals and not corner ones)
	const int x1 = std::max(update.x1 - 1,              0);
	const int y1 = std::max(update.y1 - 1,              0);
	const int x2 = std::min(update.x2 + 1, mapDims.mapxp1);
	const int y2 = std::min(update.y2 + 1, mapDims.mapyp1);


	auto& rb = RenderBuffer::GetTypedRenderBuffer<VA_TYPE_2D0>();
	rb.AssertSubmission();

	rb.AddQuadTriangles(
		{ static_cast<float>(x1), static_cast<float>(y1) },
		{ static_cast<float>(x2), static_cast<float>(y1) },
		{ static_cast<float>(x2), static_cast<float>(y2) },
		{ static_cast<float>(x1), static_cast<float>(y2) }
	);

	shadingFBO->Bind();
	glViewport(0, 0, mapDims.mapxp1, mapDims.mapyp1);

	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, heightMapTexture.GetID());

	shadingShader->Enable();

	shadingShader->SetUniform4v("groundAmbientColor", &sunLighting->groundAmbientColor.x);
	shadingShader->SetUniform4v("groundDiffuseColor", &sunLighting->groundDiffuseColor.x);
	shadingShader->SetUniform4v("lightDir", &ISky::GetSky()->GetLight()->GetLightDir().x);
	shadingShader->SetUniform3v("waterBaseColor", &waterRendering->baseColor.x);
	shadingShader->SetUniform3v("waterAbsorb", &waterRendering->absorb.x);
	shadingShader->SetUniform3v("waterMinColor", &waterRendering->minColor.x);
	shadingShader->SetUniform("waterLevel", CGround::GetWaterPlaneLevel());

	rb.DrawElements(GL_TRIANGLES);

	shadingShader->Disable();

	shadingFBO->Unbind();
	globalRendering->LoadViewport();

	glBindTexture(GL_TEXTURE_2D, 0);
}

void CSMFReadMap::SunChanged()
{
	groundDrawer->SunChanged();
}


void CSMFReadMap::ReloadTextures()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto ReloadTextureFunc = [](const std::string& texName, MapTexture& mt, float aniso = 0.0f, float lodBias = 0.0f, bool mipmaps = false) {
		/// perhaps *mt.GetIDPtr() == 0 should not be reloaded

		CBitmap bm;
		if (bm.Load(texName)) {
			TextureCreationParams tcp;
			tcp.texID = *mt.GetIDPtr();
			tcp.aniso = aniso;
			tcp.lodBias = lodBias;
			tcp.reqNumLevels = mipmaps ? 0 : 1;

			uint32_t newTexID = bm.CreateTexture(tcp);

			mt.SetRawTexID(newTexID);
			mt.SetRawSize(int2(bm.xsize, bm.ysize));
		}
	};

	ReloadTextureFunc(mapInfo->smf.grassShadingTexName  , grassShadingTex, 0.0f                      , 0.0f, true);
	ReloadTextureFunc(mapInfo->smf.detailTexName        , detailTex      , texAnisotropyLevels[false], 0.0f, true);
	ReloadTextureFunc(mapInfo->smf.minimapTexName       , minimapTex                                             );
	ReloadTextureFunc(mapInfo->smf.specularTexName      , specularTex                                            );
	ReloadTextureFunc(mapInfo->smf.blendNormalsTexName  , blendNormalsTex                                        );
	ReloadTextureFunc(mapInfo->smf.splatDistrTexName    , splatDistrTex  , texAnisotropyLevels[true] , 0.0f, true);
	ReloadTextureFunc(mapInfo->smf.splatDetailTexName   , splatDetailTex , texAnisotropyLevels[true] , 0.0f, true);
	ReloadTextureFunc(mapInfo->smf.skyReflectModTexName , skyReflectModTex                                       );
	ReloadTextureFunc(mapInfo->smf.lightEmissionTexName , lightEmissionTex                                       );
	ReloadTextureFunc(mapInfo->smf.parallaxHeightTexName, parallaxHeightTex                                      );

	for (size_t i = 0; i < mapInfo->smf.splatDetailNormalTexNames.size(); i++) {
		if (i == NUM_SPLAT_DETAIL_NORMALS)
			break;

		ReloadTextureFunc(mapInfo->smf.splatDetailNormalTexNames[i], splatNormalTextures[i], texAnisotropyLevels[true], 0.0f, true);
	}
}

int2 CSMFReadMap::GetPatch(int hmx, int hmz) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return int2 {
		std::clamp(hmx, 0, numBigTexX - 1),
		std::clamp(hmz, 0, numBigTexY - 1)
	};
}

void CSMFReadMap::BindMiniMapTextures() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// tc (0,0) - (1,1)
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, minimapTex.GetID());

	glActiveTexture(GL_TEXTURE2);

	// tc (0,0) - (isx,isy)
	if (infoTextureHandler->IsEnabled()) {
		glBindTexture(GL_TEXTURE_2D, infoTextureHandler->GetCurrentInfoTexture());
	}
	else {
		// just bind this since HAVE_INFOTEX is not available to the minimap shader
		glBindTexture(GL_TEXTURE_2D, shadingTex.GetID());
	}

	// tc (0,0) - (isx,isy)
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shadingTex.GetID());
}


void CSMFReadMap::GridVisibility(CCamera* cam, IQuadDrawer* qd, float maxDist, int quadSize, int extraSize)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (cam == nullptr) {
		// allow passing in a custom camera for grid-visibility testing
		// otherwise this culls using the state of whichever camera most
		// recently had Update() called on it
		cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_VISCUL);
		// for other cameras, KISS and just assume caller has done this
		cam->CalcFrustumLines(GetCurrMinHeight() - 100.0f, GetCurrMaxHeight() + 100.0f, SQUARE_SIZE);
	}

	// figure out the camera's own quad
	const int cx = cam->GetPos().x / (SQUARE_SIZE * quadSize);
	const int cy = cam->GetPos().z / (SQUARE_SIZE * quadSize);

	// and how many quads fit into the given maxDist
	const int drawSquare = int(maxDist / (SQUARE_SIZE * quadSize)) + 1;

	const int drawQuadsX = mapDims.mapx / quadSize;
	const int drawQuadsY = mapDims.mapy / quadSize;

	// clamp the area of quads around the camera to valid range
	const int sy  = std::clamp(cy - drawSquare, 0, drawQuadsY - 1);
	const int ey  = std::clamp(cy + drawSquare, 0, drawQuadsY - 1);
	const int sxi = std::clamp(cx - drawSquare, 0, drawQuadsX - 1);
	const int exi = std::clamp(cx + drawSquare, 0, drawQuadsX - 1);

	const CCamera::FrustumLine* negLines = cam->GetNegFrustumLines();
	const CCamera::FrustumLine* posLines = cam->GetPosFrustumLines();

	// iterate over quads row-wise between the left and right frustum lines
	for (int y = sy; y <= ey; y++) {
		int sx = sxi;
		int ex = exi;

		float xtest;
		float xtest2;

		// find the starting x-coordinate
		for (int idx = 0, cnt = negLines[4].sign; idx < cnt; idx++) {
			const CCamera::FrustumLine& fl = negLines[idx];

			xtest  = ((fl.base + fl.dir * ( y * quadSize)            ));
			xtest2 = ((fl.base + fl.dir * ((y * quadSize) + quadSize)));

			xtest = std::min(xtest, xtest2);
			xtest = std::clamp(xtest / quadSize, -1.0f, drawQuadsX * 1.0f + 1.0f);

			// increase lower bound
			if ((xtest - extraSize) > sx)
				sx = ((int) xtest) - extraSize;
		}

		// find the ending x-coordinate
		for (int idx = 0, cnt = posLines[4].sign; idx < cnt; idx++) {
			const CCamera::FrustumLine& fl = posLines[idx];

			xtest  = ((fl.base + fl.dir *  (y * quadSize)            ));
			xtest2 = ((fl.base + fl.dir * ((y * quadSize) + quadSize)));

			xtest = std::max(xtest, xtest2);
			xtest = std::clamp(xtest / quadSize, -1.0f, drawQuadsX * 1.0f + 1.0f);

			// decrease upper bound
			if ((xtest + extraSize) < ex)
				ex = ((int) xtest) + extraSize;
		}

		for (int x = sx; x <= ex; x++) {
			qd->DrawQuad(x, y);
		}
	}
}


int CSMFReadMap::GetNumFeatures() { return mapFile.GetNumFeatures(); }
int CSMFReadMap::GetNumFeatureTypes() { return mapFile.GetNumFeatureTypes(); }

void CSMFReadMap::GetFeatureInfo(MapFeatureInfo* f) { mapFile.ReadFeatureInfo(f); }

const char* CSMFReadMap::GetFeatureTypeName(int typeID) { return mapFile.GetFeatureTypeName(typeID); }


unsigned char* CSMFReadMap::GetInfoMap(const char* name, MapBitmapInfo* bmInfo)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// get size
	mapFile.GetInfoMapSize(name, bmInfo);

	if (bmInfo->width <= 0)
		return nullptr;

	unsigned char* data = new unsigned char[bmInfo->width * bmInfo->height];
	const char* texName = "";

	CBitmap infomapBM;

	switch (hashString(name)) {
		case hashString("metal"): { texName = mapInfo->smf.metalmapTexName.c_str(); } break;
		case hashString("type" ): { texName = mapInfo->smf.typemapTexName.c_str() ; } break;
		case hashString("grass"): { texName = mapInfo->smf.grassmapTexName.c_str(); } break;
		default: {
			LOG_L(L_WARNING, "[SMFReadMap::%s] unknown texture-name \"%s\"", __func__, name);
		} break;
	}

	// get data from mapinfo-override texture
	if (texName[0] != 0 && !infomapBM.LoadGrayscale(texName))
		LOG_L(L_WARNING, "[SMFReadMap::%s] cannot load override-texture \"%s\"", __func__, texName);

	if (!infomapBM.Empty()) {
		if (infomapBM.xsize == bmInfo->width && infomapBM.ysize == bmInfo->height) {
			memcpy(data, infomapBM.GetRawMem(), bmInfo->width * bmInfo->height);
			return data;
		}

		LOG_L(L_WARNING, "[SMFReadMap::%s] invalid dimensions for override-texture \"%s\": %ix%i != %ix%i",
			__func__, texName,
			infomapBM.xsize, infomapBM.ysize,
			bmInfo->width, bmInfo->height
		);
	}

	// get data from map itself
	if (mapFile.ReadInfoMap(name, data))
		return data;

	delete[] data;
	return nullptr;
}


void CSMFReadMap::FreeInfoMap(const char* name, unsigned char* data)
{
	RECOIL_DETAILED_TRACY_ZONE;
	delete[] data;
}


void CSMFReadMap::ConfigureTexAnisotropyLevels()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!GLAD_GL_EXT_texture_filter_anisotropic) {
		texAnisotropyLevels[false] = 0.0f;
		texAnisotropyLevels[ true] = 0.0f;
		return;
	}

	const std::string cfgKeys[2] = {"SMFTexAniso", "SSMFTexAniso"};

	for (unsigned int i = 0; i < 2; i++) {
		texAnisotropyLevels[i] = std::min(configHandler->GetFloat(cfgKeys[i]), globalRendering->maxTexAnisoLvl);
		texAnisotropyLevels[i] *= (texAnisotropyLevels[i] >= 1.0f); // disable AF if less than 1
	}
}


bool CSMFReadMap::SetLuaTexture(const MapTextureData& td) {
	RECOIL_DETAILED_TRACY_ZONE;
	const unsigned int num = std::clamp(int(td.num), 0, NUM_SPLAT_DETAIL_NORMALS - 1);

	switch (td.type) {
		case MAP_BASE_GRASS_TEX: { grassShadingTex.SetLuaTexture(td); } break;
		case MAP_BASE_DETAIL_TEX: { detailTex.SetLuaTexture(td); } break;
		case MAP_BASE_MINIMAP_TEX: { minimapTex.SetLuaTexture(td); } break;
		case MAP_BASE_SHADING_TEX: { shadingTex.SetLuaTexture(td); } break;
		case MAP_BASE_NORMALS_TEX: { normalsTex.SetLuaTexture(td); } break;

		case MAP_SSMF_SPECULAR_TEX: { specularTex.SetLuaTexture(td); } break;
		case MAP_SSMF_NORMALS_TEX: { blendNormalsTex.SetLuaTexture(td); } break;

		case MAP_SSMF_SPLAT_DISTRIB_TEX: { splatDistrTex.SetLuaTexture(td); } break;
		case MAP_SSMF_SPLAT_DETAIL_TEX: { splatDetailTex.SetLuaTexture(td); } break;
		case MAP_SSMF_SPLAT_NORMAL_TEX: { splatNormalTextures[num].SetLuaTexture(td); } break;

		case MAP_SSMF_SKY_REFLECTION_TEX: { skyReflectModTex.SetLuaTexture(td); } break;
		case MAP_SSMF_LIGHT_EMISSION_TEX: { lightEmissionTex.SetLuaTexture(td); } break;
		case MAP_SSMF_PARALLAX_HEIGHT_TEX: { parallaxHeightTex.SetLuaTexture(td); } break;

		default: {
			return false;
		} break;
	}

	groundDrawer->UpdateRenderState();
	return true;
}

void CSMFReadMap::InitGroundDrawer() { groundDrawer = new CSMFGroundDrawer(this); }
void CSMFReadMap::KillGroundDrawer() { spring::SafeDelete(groundDrawer); }

// not placed in header since type CSMFGroundDrawer is only forward-declared there
inline CBaseGroundDrawer* CSMFReadMap::GetGroundDrawer() { return groundDrawer; }

