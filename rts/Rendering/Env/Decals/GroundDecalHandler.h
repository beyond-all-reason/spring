/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <variant>
#include <limits>
#include <optional>

#include "GroundDecal.h"
#include "Rendering/Env/IGroundDecalDrawer.h"
#include "Rendering/GL/VBO.h"
#include "Rendering/GL/VAO.h"
#include "Rendering/Textures/TextureRenderAtlas.h"
#include "System/EventClient.h"
#include "System/UnorderedMap.hpp"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Projectiles/ExplosionListener.h"

class CSolidObject;
class CUnit;
struct SolidObjectDecalType;
class CTextureAtlas;
class CSMFGroundDrawer;
class GhostSolidObject;

namespace Shader {
	struct IProgramObject;
}

class CGroundDecalHandler: public IGroundDecalDrawer, public CEventClient, public IExplosionListener
{
public:
	class DecalUpdateList {
	public:
		using IteratorPair = std::pair<std::vector<bool>::iterator, std::vector<bool>::iterator>;
	public:
		DecalUpdateList()
			: updateList()
			, changed(true)
		{}

		void Resize(size_t newSize) { updateList.resize(newSize); SetNeedUpdateAll(); }
		void Reserve(size_t reservedSize) { updateList.reserve(reservedSize); }

		void SetUpdate(const IteratorPair& it);
		void SetUpdate(size_t offset);

		void SetNeedUpdateAll();
		void ResetNeedUpdateAll();

		void EmplaceBackUpdate();

		bool NeedUpdate() const { return changed; }

		std::optional<IteratorPair> GetNext(const std::optional<IteratorPair>& prev = std::nullopt);
		std::pair<size_t, size_t> GetOffsetAndSize(const IteratorPair& it);
	private:
		std::vector<bool> updateList;
		bool changed;
	};
public:
	CGroundDecalHandler();
	~CGroundDecalHandler();

	void OnDecalLevelChanged() override {}
private:
	void BindAtlasTextures();
	void BindCommonTextures();
	void UnbindTextures();
	void AddExplosion(float3 pos, float damage, float radius);
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
			|| (eventName == "GameFrame")
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

	void GameFrame(int frameNum) override;

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

	size_t CreateLuaDecal() override { return 0; };
	GroundDecal* GetDecalByIdx(size_t idx) override { return nullptr; };
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
	void AddGroundScarTextures();
	void AddGroundTrackTextures();
	void AddFallbackTextures();
private:
	struct UnitMinMaxHeight {
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

	DecalUpdateList decalsUpdateList;

	VBO instVBO;
	VAO vao;

	CSMFGroundDrawer* smfDrawer;

	int lastProcessedGameFrame;
	bool highQuality = false;

	static constexpr uint32_t TRACKS_UPDATE_RATE = 4u;
};