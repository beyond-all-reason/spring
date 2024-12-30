/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef READ_MAP_H
#define READ_MAP_H

#include <array>
#include <vector>

#include "MapTexture.h"
#include "MapDimensions.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/GlobalSynced.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/creg/creg_cond.h"
#include "System/Misc/RectangleOverlapHandler.h"

#define USE_HEIGHTMAP_DIGESTS

class CCamera;
class CUnit;
class CSolidObject;
class CBaseGroundDrawer;


struct MapFeatureInfo
{
	float3 pos;
	/// index to one of the strings above
	int featureType;
	float rotation;
};


struct MapBitmapInfo
{
	MapBitmapInfo() : width(0), height(0) {}
	MapBitmapInfo(int w, int h) : width(w), height(h) {}

	int width;
	int height;
};



enum {
	// base textures
	MAP_BASE_GRASS_TEX           =  0,
	MAP_BASE_DETAIL_TEX          =  1,
	MAP_BASE_MINIMAP_TEX         =  2,
	MAP_BASE_SHADING_TEX         =  3,
	MAP_BASE_NORMALS_TEX         =  4,

	// SSMF textures
	MAP_SSMF_NORMALS_TEX         =  5,
	MAP_SSMF_SPECULAR_TEX        =  6,

	MAP_SSMF_SPLAT_DISTRIB_TEX   =  7,
	MAP_SSMF_SPLAT_DETAIL_TEX    =  8,
	MAP_SSMF_SPLAT_NORMAL_TEX    =  9,

	MAP_SSMF_SKY_REFLECTION_TEX  = 10,
	MAP_SSMF_LIGHT_EMISSION_TEX  = 11,
	MAP_SSMF_PARALLAX_HEIGHT_TEX = 12,
};



class CReadMap
{
protected:
	/// called by implementations of CReadMap
	void Initialize();

	virtual void UpdateHeightMapUnsynced(const SRectangle&) = 0;
	virtual void UpdateHeightMapUnsyncedPost() = 0;
public:
	//OK since it's loaded with SerializeObjectInstance
	CR_DECLARE_STRUCT(CReadMap)

	static CReadMap* LoadMap(const std::string& mapname);
	static inline uint8_t EncodeHeight(const float h) {
		return std::max(0, 255 + int(10.0f * h));
	}

	/// creg serialize callback
	void Serialize(creg::ISerializer* s);

private:
	void SerializeMapChangesBeforeMatch(creg::ISerializer* s);
	void SerializeMapChangesDuringMatch(creg::ISerializer* s);
	void SerializeMapChanges(creg::ISerializer* s, const float* refHeightMap, float* modifiedHeightMap);
	void SerializeTypeMap(creg::ISerializer* s);

public:
	void PostLoad();

	void InitHeightMapDigestVectors(const int2 losMapSize);

	/**
	 * calculates derived heightmap information
	 * such as normals, centerheightmap and slopemap
	 */
	void UpdateHeightMapSynced(const SRectangle& hgtMapRect);
	void UpdateLOS(const SRectangle& hgtMapRect);
	void BecomeSpectator();
	void UpdateDraw(bool firstCall);

	virtual ~CReadMap();

	virtual void ReloadTextures() = 0;

	virtual void Update() { UpdateHeightBounds(gs->frameNum); }
	virtual void UpdateShadingTexture() {}

	virtual void InitGroundDrawer() = 0;
	virtual void KillGroundDrawer() = 0;
	virtual CBaseGroundDrawer* GetGroundDrawer() { return 0; }


	virtual unsigned int GetGrassShadingTexture() const { return 0; }
	virtual unsigned int GetMiniMapTexture() const { return 0; }
	/**
	 * a texture with RGB for shading and A for height
	 * (0 := above water; 1-255 := under water = 255+height*10)
	 */
	virtual unsigned int GetShadingTexture() const = 0;

	virtual unsigned int GetTexture(unsigned int type, unsigned int num = 0) const { return 0; }
	virtual int2 GetTextureSize(unsigned int type, unsigned int num = 0) const { return int2(0, 0); }

	virtual bool SetLuaTexture(const MapTextureData&) { return false; }

	/// binds textures used to draw the minimap in a quad with extents (0,0)-(1,1))
	virtual void BindMiniMapTextures() const = 0;

	/// Feature creation
	virtual int GetNumFeatures() = 0;
	virtual int GetNumFeatureTypes() = 0;
	/// Returns MapFeatureInfo[GetNumFeatures()]
	virtual void GetFeatureInfo(MapFeatureInfo* f) = 0;
	virtual const char* GetFeatureTypeName(int typeID) = 0;

	/**
	 * Infomaps (such as metal map, grass map, ...),
	 * handling them with a string as type seems flexible...
	 * Some map types:
	 *   "metal"  -  metalmap
	 *   "grass"  -  grassmap
	 */
	virtual unsigned char* GetInfoMap(const char* name, MapBitmapInfo* bm) = 0;
	virtual void FreeInfoMap(const char* name, unsigned char* data) = 0;

	/// Determine visibility for a rectangular grid
	/// call ResetState for statically allocated drawer objects
	struct IQuadDrawer
	{
		virtual ~IQuadDrawer() {}
		virtual void ResetState() = 0;
		virtual void DrawQuad(int x, int y) = 0;
	};
	virtual void GridVisibility(CCamera* cam, IQuadDrawer* cb, float maxDist, int quadSize, int extraSize = 0) = 0;


	/// synced only
	const float* GetUnmodifiedHeightMapSynced() const { return unmodifiedHeightMap.data(); }
	const float* GetOriginalHeightMapSynced() const { return originalHeightMap.data(); }
	const float* GetCenterHeightMapSynced() const { return centerHeightMapLods[0].data(); }
	const float* GetCenterHeightMapSyncedLod(uint32_t lod) const { return centerHeightMapLods[lod].data(); }
	const float* GetMaxHeightMapSynced() const { return maxHeightMap.data(); }
	const float* GetSlopeMapSynced() const { return slopeMap.data(); }
	const uint8_t* GetTypeMapSynced() const { return typeMap.data(); }
	      uint8_t* GetTypeMapSynced()       { return typeMap.data(); }
	const float3* GetCenterNormals2DSynced()  const { return centerNormals2D.data(); }

	/// unsynced only
	const float3* GetVisVertexNormalsUnsynced() const { return visVertexNormals.data(); }

	/// synced versions
	const float* GetCornerHeightMapSynced() const { return cornerHeightMapSynced.data(); }
	const float3* GetFaceNormalsSynced()    const { return faceNormalsSynced.data(); }
	const float3* GetCenterNormalsSynced()  const { return centerNormalsSynced.data(); }

	/// unsynced versions
	const float* GetCornerHeightMapUnsynced() const { return cornerHeightMapUnsynced.data(); }
	const float3* GetFaceNormalsUnsynced()    const { return faceNormalsUnsynced.data(); }
	const float3* GetCenterNormalsUnsynced()  const { return centerNormalsUnsynced.data(); }

	/// shared interface
	const float* GetSharedCornerHeightMap(bool synced) const;
	const float* GetSharedCenterHeightMap(bool synced) const;
	const float3* GetSharedFaceNormals(bool synced) const;
	const float3* GetSharedCenterNormals(bool synced) const;
	const float* GetSharedSlopeMap(bool synced) const;

	// Misc
	void CopySyncedToUnsynced();

	/// if you modify the heightmap through these, call UpdateHeightMapSynced
	float SetHeight(const int idx, const float h, const int add = 0);
	float AddHeight(const int idx, const float a);

	/// These will not modify the current heightmap, only the original
	float SetOriginalHeight(const int idx, const float h, const int add = 0);
	float AddOriginalHeight(const int idx, const float a);

	float GetInitMinHeight() const { return initHeightBounds.x; }
	float GetCurrMinHeight() const { return currHeightBounds.x; }
	float GetInitMaxHeight() const { return initHeightBounds.y; }
	float GetCurrMaxHeight() const { return currHeightBounds.y; }
	float GetInitAvgHeight() const { return ((GetInitMinHeight() + GetInitMaxHeight()) * 0.5f); }
	float GetCurrAvgHeight() const { return ((GetCurrMinHeight() + GetCurrMaxHeight()) * 0.5f); }
	float GetBoundingRadius() const { return boundingRadius; }

	bool IsUnderWater() const { return (currHeightBounds.y <  0.0f); }
	bool IsAboveWater() const { return (currHeightBounds.x >= 0.0f); }

	bool HasVisibleWater() const;
	bool HasOnlyVoidWater() const;

	unsigned int GetMapChecksum() const { return mapChecksum; }
	unsigned int CalcHeightmapChecksum();
	unsigned int CalcTypemapChecksum();

	void UpdateHeightBounds();

	bool GetHeightMapUpdated() const { return hmUpdated; }

	virtual int2 GetPatch(int hmx, int hmz) const = 0;
	virtual const float3& GetUnsyncedHeightInfo(int patchX, int patchZ) const = 0;
private:
	void InitHeightBounds();
	void LoadOriginalHeightMapAndChecksum();
	void UpdateHeightBounds(int syncFrame);
	void UpdateTempHeightBoundsSIMD(size_t begin, size_t end);

	void UpdateCenterHeightmap(const SRectangle& rect, bool initialize) const;
	void UpdateMipHeightmaps(const SRectangle& rect, bool initialize);
	void UpdateFaceNormals(const SRectangle& rect, bool initialize);
	void UpdateSlopemap(const SRectangle& rect, bool initialize);

	inline void HeightMapUpdateLOSCheck(const SRectangle& hgtMapRect);
	inline bool HasHeightMapViewChanged(const int2 losMapPos);

	float SetHeightValue(float& heightRef, const int idx, const float h, const int add = 0);

public:
	/// number of heightmap mipmaps, including full resolution
	static constexpr int NUM_HM_LODS = 7;
	static constexpr int32_t PATCH_SIZE = 128;
protected:
	// note: intentionally declared static, s.t. repeated reloading to the same
	// (or any smaller) map does not fragment the heap which invites bad_alloc's
	static std::vector<float> unmodifiedHeightMap;			// raw heightMap unmodified from the map file
	static std::vector<float> originalHeightMap;        //< size: (mapx+1)*(mapy+1) (per vertex) [SYNCED, does NOT update on terrain deformation]
	static std::vector<float> maxHeightMap;			// map for sea/hover to catch coast lines with sharp vertical changes so they don't try to climb the cliff.

	//< size: (mapx  )*(mapy  ) (per face) [SYNCED, updates on terrain deformation]
	// every next LOD is 2x2 smaller
	static std::array<std::vector<float>, NUM_HM_LODS> centerHeightMapLods;

	static std::vector<float3> visVertexNormals;       //< size:  (mapx + 1) * (mapy + 1), contains one vertex normal per corner-heightmap pixel [UNSYNCED]
	static std::vector<float3> faceNormalsSynced;      //< size: 2*mapx      *  mapy     , contains 2 normals per quad -> triangle strip [SYNCED]
	static std::vector<float3> faceNormalsUnsynced;    //< size: 2*mapx      *  mapy     , contains 2 normals per quad -> triangle strip [UNSYNCED]
	static std::vector<float3> centerNormalsSynced;    //< size:   mapx      *  mapy     , contains 1 interpolated normal per quad, same as (facenormal0+facenormal1).Normalize()) [SYNCED]
	static std::vector<float3> centerNormalsUnsynced;  //< size:   mapx      *  mapy

	static std::vector<float> slopeMap;                //< size: (mapx/2)    * (mapy/2)  , same as 1.0 - interpolate(centernomal[i]).y [SYNCED]
	static std::vector<uint8_t> typeMap;               //< size: (mapx/2)    * (mapy/2)
	static std::vector<float3> centerNormals2D;        //< size:   mapx      *  mapy

	static std::vector<float> cornerHeightMapSynced;   //< size:  (mapx + 1) * (mapy + 1)
	static std::vector<float> cornerHeightMapUnsynced; //< size:  (mapx + 1) * (mapy + 1)

	CRectangleOverlapHandler unsyncedHeightMapUpdates;

	std::vector<float3> unsyncedHeightInfo; // per 128x128 HM patch
private:
	/// these are not "digests", just simple rolling counters
	/// for each LOS-map square the counter value indicates how many times
	/// the synced heightmap block of squares corresponding to it has been
	/// changed, s.t. UHM updates are only pushed when necessary
	static std::vector<uint8_t>   syncedHeightMapDigests;
	static std::vector<uint8_t> unsyncedHeightMapDigests;

	unsigned int mapChecksum = 0;

	bool processingHeightBounds = false;
	bool hmUpdated = false;

	float2 initHeightBounds; //< initial minimum- and maximum-height (before any deformations)
	float2 tempHeightBounds; //< temporary minimum- and maximum-height
	float2 currHeightBounds; //< current minimum- and maximum-height

	float boundingRadius = 0.0f;
};


extern CReadMap* readMap;
extern MapDimensions mapDims;

inline float CReadMap::AddHeight(const int idx, const float a) { return SetHeight(idx, a, 1); }
inline float CReadMap::SetHeight(const int idx, const float h, const int add) {
	return SetHeightValue(cornerHeightMapSynced[idx], idx, h, add);
}

inline float CReadMap::AddOriginalHeight(const int idx, const float a) { return SetOriginalHeight(idx, a, 1); }
inline float CReadMap::SetOriginalHeight(const int idx, const float h, const int add) {
	return SetHeightValue(originalHeightMap[idx], idx, h, add);
}

inline float CReadMap::SetHeightValue(float& heightRef, const int idx, const float h, const int add) {
	// add=0 <--> x = x*0 + h =   h
	// add=1 <--> x = x*1 + h = x+h
	float newHeight = heightRef * add + h;
	hmUpdated |= (newHeight != heightRef);
	return (heightRef = newHeight);
}


static inline float3 CornerSqrToPosRaw(const float* hm, int sqx, int sqz) { return {sqx * SQUARE_SIZE * 1.0f, hm[(sqz * mapDims.mapxp1) + sqx], sqz * SQUARE_SIZE * 1.0f}; }
static inline float3 CenterSqrToPosRaw(const float* hm, int sqx, int sqz) { return {sqx * SQUARE_SIZE * 1.0f, hm[(sqz * mapDims.mapx  ) + sqx], sqz * SQUARE_SIZE * 1.0f}; }

static inline float3 CornerSqrToPos(const float* hm, int sqx, int sqz) { return (CornerSqrToPosRaw(hm, std::clamp(sqx, 0, mapDims.mapx  ), std::clamp(sqz, 0, mapDims.mapy  ))); }
static inline float3 CenterSqrToPos(const float* hm, int sqx, int sqz) { return (CenterSqrToPosRaw(hm, std::clamp(sqx, 0, mapDims.mapxm1), std::clamp(sqz, 0, mapDims.mapym1))); }


static inline float3 CornerSquareToFloat3(int sqx, int sqz) { return (CornerSqrToPosRaw(readMap->GetCornerHeightMapSynced(), sqx, sqz)); }
static inline float3       SquareToFloat3(int sqx, int sqz) { return (CenterSqrToPosRaw(readMap->GetCenterHeightMapSynced(), sqx, sqz)); }

static inline float3 CornerSquareToFloat3(int2 sqr) { return (CornerSquareToFloat3(sqr.x, sqr.y)); }
static inline float3       SquareToFloat3(int2 sqr) { return (      SquareToFloat3(sqr.x, sqr.y)); }


#endif /* READ_MAP_H */
