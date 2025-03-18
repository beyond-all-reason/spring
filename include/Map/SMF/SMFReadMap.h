/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <array>
#include <memory>

#include "SMFMapFile.h"
#include "Map/ReadMap.h"
#include "System/EventClient.h"
#include "System/type2.h"


class CSMFGroundDrawer;
class FBO;
namespace Shader {
	struct IProgramObject;
}

class CSMFReadMap : public CReadMap, public CEventClient
{
public:
	// CEventClient interface
	int GetReadAllyTeam() const override { return AllAccessTeam; }
	bool WantsEvent(const std::string& eventName) override {
		return eventName == "SunChanged";
	}

	void SunChanged() override;

public:

	CSMFReadMap(const std::string& mapName);
	// note: textures are auto-deleted
	~CSMFReadMap();

	void ReloadTextures() override;

	void UpdateShadingTexture() override;
	int2 GetPatch(int hmx, int hmz) const override;
	const float3& GetUnsyncedHeightInfo(int patchX, int patchZ) const override { return unsyncedHeightInfo[patchZ * numBigTexX + patchX]; }
public:
	bool SetLuaTexture(const MapTextureData& td) override;

public:
	uint32_t GetTexture(uint32_t type, uint32_t num = 0) const override {
		uint32_t texID = 0;

		switch (type) {
			case MAP_BASE_GRASS_TEX: { texID = GetGrassShadingTexture(); } break;
			case MAP_BASE_DETAIL_TEX: { texID = GetDetailTexture(); } break;
			case MAP_BASE_MINIMAP_TEX: { texID = GetMiniMapTexture(); } break;
			case MAP_BASE_SHADING_TEX: { texID = GetShadingTexture(); } break;
			case MAP_BASE_NORMALS_TEX: { texID = GetNormalsTexture(); } break;

			case MAP_SSMF_NORMALS_TEX: { texID = GetBlendNormalsTexture(); } break;
			case MAP_SSMF_SPECULAR_TEX: { texID = GetSpecularTexture(); } break;

			case MAP_SSMF_SPLAT_DISTRIB_TEX: { texID = GetSplatDistrTexture(); } break;
			case MAP_SSMF_SPLAT_DETAIL_TEX: { texID = GetSplatDetailTexture(); } break;
			case MAP_SSMF_SPLAT_NORMAL_TEX: { texID = GetSplatNormalTexture(num); } break;

			case MAP_SSMF_SKY_REFLECTION_TEX: { texID = GetSkyReflectModTexture(); } break;
			case MAP_SSMF_LIGHT_EMISSION_TEX: { texID = GetLightEmissionTexture(); } break;
			case MAP_SSMF_PARALLAX_HEIGHT_TEX: { texID = GetParallaxHeightTexture(); } break;

			default: {} break;
		}

		return texID;
	}

	int2 GetTextureSize(uint32_t type, uint32_t num = 0) const override {
		int2 size;

		// TODO:
		//   for now, always return the raw texture size even if HasLuaTex
		//   this means any Lua-set textures must have the same dimensions
		//   as their originals (not an unreasonable limitation except for
		//   custom shaders)
		switch (type) {
			case MAP_BASE_GRASS_TEX: { size = grassShadingTex.GetRawSize(); } break;
			case MAP_BASE_DETAIL_TEX: { size = detailTex.GetRawSize(); } break;
			case MAP_BASE_MINIMAP_TEX: { size = minimapTex.GetRawSize(); } break;
			case MAP_BASE_SHADING_TEX: { size = shadingTex.GetRawSize(); } break;
			case MAP_BASE_NORMALS_TEX: { size = normalsTex.GetRawSize(); } break;

			case MAP_SSMF_NORMALS_TEX: { size = blendNormalsTex.GetRawSize(); } break;
			case MAP_SSMF_SPECULAR_TEX: { size = specularTex.GetRawSize(); } break;

			case MAP_SSMF_SPLAT_DISTRIB_TEX: { size = splatDistrTex.GetRawSize(); } break;
			case MAP_SSMF_SPLAT_DETAIL_TEX: { size = splatDetailTex.GetRawSize(); } break;
			case MAP_SSMF_SPLAT_NORMAL_TEX: { size = splatNormalTextures[num].GetRawSize(); } break;

			case MAP_SSMF_SKY_REFLECTION_TEX: { size = skyReflectModTex.GetRawSize(); } break;
			case MAP_SSMF_LIGHT_EMISSION_TEX: { size = lightEmissionTex.GetRawSize(); } break;
			case MAP_SSMF_PARALLAX_HEIGHT_TEX: { size = parallaxHeightTex.GetRawSize(); } break;
		}

		return size;
	}

public:
	uint32_t GetGrassShadingTexture() const override { return grassShadingTex.GetID(); }
	uint32_t GetMiniMapTexture() const override { return minimapTex.GetID(); }
	uint32_t GetDetailTexture() const { return detailTex.GetID(); }
	uint32_t GetShadingTexture() const override { return shadingTex.GetID(); }
	uint32_t GetHeightMapTexture() const override { return heightMapTexture.GetID(); }
	uint32_t GetNormalsTexture() const  { return normalsTex.GetID(); }

	uint32_t GetSpecularTexture() const { return specularTex.GetID(); }
	uint32_t GetBlendNormalsTexture() const { return blendNormalsTex.GetID(); }

	uint32_t GetSplatDistrTexture() const { return splatDistrTex.GetID(); }
	uint32_t GetSplatDetailTexture() const { return splatDetailTex.GetID(); }
	uint32_t GetSplatNormalTexture(int i) const { return splatNormalTextures[i].GetID(); }

	uint32_t GetSkyReflectModTexture() const { return skyReflectModTex.GetID(); }
	uint32_t GetLightEmissionTexture() const { return lightEmissionTex.GetID(); }
	uint32_t GetParallaxHeightTexture() const { return parallaxHeightTex.GetID(); }

	const MapTexture& GetHeightMapTextureObj() const override { return heightMapTexture; }
public:
	void BindMiniMapTextures() const override;
	void GridVisibility(CCamera* cam, IQuadDrawer* cb, float maxDist, int quadSize, int extraSize = 0) override;

	void InitGroundDrawer() override;
	void KillGroundDrawer() override;

	int GetNumFeatureTypes() override;
	int GetNumFeatures() override;
	void GetFeatureInfo(MapFeatureInfo* f) override; // returns all feature info in MapFeatureInfo[NumFeatures]
	const char* GetFeatureTypeName(int typeID) override;

	unsigned char* GetInfoMap(const char* name, MapBitmapInfo* bm) override;
	void FreeInfoMap(const char* name, unsigned char* data) override;


	// NOTE: do not use, just here for backward compatibility with SMFGroundTextures.cpp
	inline CSMFMapFile& GetMapFile() { return mapFile; }
	inline CBaseGroundDrawer* GetGroundDrawer() override;


	void ConfigureTexAnisotropyLevels();
	float GetTexAnisotropyLevel(bool ssmf) const { return texAnisotropyLevels[ssmf]; }

	bool HaveSplatNormalTexture() const {
		if (splatNormalTextures[0].GetID() != 0) return true;
		if (splatNormalTextures[1].GetID() != 0) return true;
		if (splatNormalTextures[2].GetID() != 0) return true;
		if (splatNormalTextures[3].GetID() != 0) return true;
		return false;
	}

private:
	void UpdateHeightMapUnsynced(const SRectangle&) override;
	void UpdateHeightMapUnsyncedPost() override;

	void ParseHeader();
	void LoadHeightMap();
	void LoadMinimap();
	void CreateSpecularTex();
	void CreateSplatDetailTextures();
	void CreateGrassTex();
	void CreateDetailTex();
	void CreateShadingTex();
	void CreateNormalTex();
	void CreateHeightMapTex();
	void CreateShadingGL();

	void UpdateCornerHeightMapUnsynced(const SRectangle& update);
	void UpdateHeightMapTexture(const SRectangle& update);
	void UpdateHeightBoundsUnsynced(const SRectangle& update);
	void UpdateFaceNormalsUnsynced(const SRectangle& update);
	void UpdateVisNormalsAndShadingTexture(const SRectangle& update);

	void ParseSMD(std::string filename);

public:
	// constants
	static constexpr int tileScale     = 4;
	static constexpr int bigSquareSize = 32 * tileScale;
	static constexpr int NUM_SPLAT_DETAIL_NORMALS = 4;

	// globals for SMFGround{Drawer, Textures}
	int numBigTexX;
	int numBigTexY;
	int bigTexSize;
	int tileMapSizeX;
	int tileMapSizeY;
	int tileCount;
	int mapSizeX;
	int mapSizeZ;
	int maxHeightMapIdx;
	int heightMapSizeX;

private:
	// note: intentionally declared static (see ReadMap)
	static CSMFMapFile mapFile;

	static std::vector<float> cornerHeightMapSynced;
	static std::vector<float> cornerHeightMapUnsynced;
private:
	CSMFGroundDrawer* groundDrawer = nullptr;

private:
	std::unique_ptr<FBO> shadingFBO;
	Shader::IProgramObject* shadingShader = nullptr;

	MapTexture grassShadingTex;       // specifies grass-blade modulation color (defaults to minimapTex)
	MapTexture detailTex;             // supplied by the map
	MapTexture minimapTex;            // supplied by the map
	MapTexture shadingTex;            // holds precomputed dot(lightDir, vertexNormal) values
	MapTexture normalsTex;            // holds vertex normals in RA32F internal format (GL_RA + GL_FLOAT)

	MapTexture specularTex;           // supplied by the map, moderates specular contribution
	MapTexture blendNormalsTex;       // tangent-space offset normals

	MapTexture splatDistrTex;         // specifies the per-channel distribution of splatDetailTex (map-wide, overrides detailTex)
	MapTexture splatDetailTex;        // contains per-channel separate greyscale detail-textures (overrides detailTex)

	// contains RGBA texture with RGB channels containing normals;
	// alpha contains greyscale diffuse for splat detail normals
	// if haveDetailNormalDiffuseAlpha (overrides detailTex)
	std::array<MapTexture, NUM_SPLAT_DETAIL_NORMALS> splatNormalTextures;

	MapTexture skyReflectModTex;      // modulates sky-reflection RGB intensities (must be the same size as specularTex)
	MapTexture lightEmissionTex;
	MapTexture parallaxHeightTex;

	MapTexture heightMapTexture;
private:
	float texAnisotropyLevels[2] = {0.0f, 0.0f};

	bool haveSpecularTexture           = false;
	bool haveSplatDetailDistribTexture = false; // true if we have both splatDetailTex and splatDistrTex
	bool haveSplatNormalDistribTexture = false; // true if we have splatDistrTex and at least one splat[Detail]NormalTex

	bool shadingTexUpdateNeeded = false;
};