/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <variant>
#include <limits>
#include <optional>

#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/DepthBufferCopy.h"
#include "Rendering/Textures/TextureRenderAtlas.h"
#include "Rendering/Common/UpdateList.h"
#include "System/EventClient.h"
#include "System/UnorderedMap.hpp"
#include "System/creg/creg.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Projectiles/ExplosionListener.h"

class CSolidObject;
class CUnit;
struct SolidObjectDecalType;
class CTextureAtlas;
class CSMFGroundDrawer;
class GhostSolidObject;
class CColorMap;

namespace Shader {
	struct IProgramObject;
}

class CGroundDecalHandler: public IGroundDecalDrawer, public CEventClient, public IExplosionListener
{
	CR_DECLARE_DERIVED(CGroundDecalHandler)
	CR_DECLARE_SUB(UnitMinMaxHeight)
public:
	CGroundDecalHandler();
	~CGroundDecalHandler() override;
private:
	struct AddExplosionInfo {
		float3 pos;
		float3 projDir;
		float damage;
		float radius;
		float maxHeightDiff;
		const WeaponDef* wd;
	};
private:
	void BindAtlasTextures();
	void BindCommonTextures();
	void UnbindTextures();
	void AddExplosion(AddExplosionInfo&& explInfo);
	void MoveSolidObject(const CSolidObject* object, const float3& pos);
public:
	// CEventClient
	bool WantsEvent(const std::string& eventName) override {
		return
			   (eventName == "RenderUnitCreated")
			|| (eventName == "RenderUnitDestroyed")
			|| (eventName == "RenderFeatureCreated")
			|| (eventName == "RenderFeatureDestroyed")
			|| (eventName == "UnitMoved")
			|| (eventName == "UnitLoaded")
			|| (eventName == "UnitUnloaded")
			|| (eventName == "GameFramePost")
			|| (eventName == "SunChanged")
			|| (eventName == "ViewResize");
	}

	void ConfigNotify(const std::string& key, const std::string& value);

	bool GetFullRead() const override { return true; }
	int GetReadAllyTeam() const override { return AllAccessTeam; }

	void RenderUnitCreated(const CUnit*, int cloaked) override;
	void RenderUnitDestroyed(const CUnit*) override;
	void RenderFeatureCreated(const CFeature* feature) override;
	void RenderFeatureDestroyed(const CFeature* feature) override;
	void FeatureMoved(const CFeature* feature, const float3& oldpos) override;
	void UnitMoved(const CUnit* unit) override;
	void UnitLoaded(const CUnit* unit, const CUnit* transport) override;
	void UnitUnloaded(const CUnit* unit, const CUnit* transport) override;

	void GameFramePost(int frameNum) override;

	void SunChanged() override;
	void ViewResize() override;

	// IExplosionListener
	void ExplosionOccurred(const CExplosionParams& event) override;

	// IGroundDecalDrawer
	void ReloadTextures() override;
	void DumpAtlasTextures() override;

	void Draw() override;

	void AddSolidObject(const CSolidObject* object) override;
	void ForceRemoveSolidObject(const CSolidObject* object) override;

	void GhostCreated(const CSolidObject* object, const GhostSolidObject* gb) override;
	void GhostDestroyed(const GhostSolidObject* gb) override;

	uint32_t CreateLuaDecal() override;
	bool DeleteLuaDecal(uint32_t id) override;
	      GroundDecal* GetDecalById(uint32_t id)       override;
	const GroundDecal* GetDecalById(uint32_t id) const override;
	bool SetDecalTexture(uint32_t id, const std::string& texName, bool mainTex) override;
	std::string GetDecalTexture(uint32_t id, bool mainTex) const override;
	const std::vector<std::string> GetDecalTextures(bool mainTex) const override;
	const CSolidObject* GetDecalSolidObjectOwner(uint32_t id) const override;

	void SetUnitLeaveTracks(CUnit* unit, bool leaveTracks) override;

	void PostLoad();
private:
	static void BindVertexAtrribs();
	static void UnbindVertexAtrribs();

	uint32_t GetDepthBufferTextureTarget() const;

	void GenerateAtlasTextures();
	void ReloadDecalShaders();

	void AddTexToAtlas(const std::string& name, const std::string& filename, bool mainTex, bool convertOldBMP);

	void AddTrack(const CUnit* unit, const float3& newPos, bool forceEval = false);

	void RemoveSolidObject(const CSolidObject* object, const GhostSolidObject* gb);

	void CompactDecalsVector(int frameNum);

	void UpdateDecalsVisibility();

	void AddBuildingDecalTextures();
	void AddTexturesFromTable();
	void AddGroundTrackTextures();
	void AddFallbackTextures();

	uint32_t GetNextId();
private:
	struct UnitMinMaxHeight {
		CR_DECLARE_STRUCT(UnitMinMaxHeight)
		UnitMinMaxHeight()
			: min(std::numeric_limits<float>::max()   )
			, max(std::numeric_limits<float>::lowest())
		{}
		float min;
		float max;
	};
	int maxUniqueScars;

	std::unique_ptr<CTextureRenderAtlas> atlasMain;
	std::unique_ptr<CTextureRenderAtlas> atlasNorm;

	Shader::IProgramObject* decalShader;

	using DecalOwner = std::variant<const CSolidObject*, const GhostSolidObject*>;
	spring::unordered_map<DecalOwner, size_t, std::hash<DecalOwner>> decalOwners; // for tracks, plates and ghosts
	spring::unordered_map<int, UnitMinMaxHeight> unitMinMaxHeights; // for tracks
	spring::unordered_map<uint32_t, size_t> idToPos;
	spring::unordered_map<uint32_t, std::tuple<const CColorMap*, std::pair<size_t, size_t>>> idToCmInfo;

	UpdateList decalsUpdateList;

	uint32_t nextId;
	std::vector<uint32_t> freeIds;

	VBO instVBO;
	VAO vao;

	CSMFGroundDrawer* smfDrawer;

	bool highQuality = false;
	ScopedDepthBufferCopy sdbc;

	static constexpr uint32_t TRACKS_UPDATE_RATE = 4u;
};