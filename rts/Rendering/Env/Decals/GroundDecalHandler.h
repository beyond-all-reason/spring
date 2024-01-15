/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <limits>

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
	CGroundDecalHandler();
	~CGroundDecalHandler();

	void OnDecalLevelChanged() override {}
private:
	void BindGroundAtlasTextures();
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
	//void FeatureMoved(const CFeature* feature, const float3& oldpos) override;
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

	void AddTexToGroundAtlas(const std::string& name, bool mainTex);
	void AddTexToAtlas(const std::string& name, bool mainTex);
	void AddTexToAtlas(const std::string& name, const std::string& filename, const std::string& filenameAlt, bool mainTex);

	void AddTrack(const CUnit* unit, const float3& newPos);

	void RemoveSolidObject(const CSolidObject* object, const GhostSolidObject* gb);

	void UpdateTemporaryDecalsVector(int frameNum);

	void AddBuildingDecalTextures();
	void AddGroundScarTextures();
	void AddGroundTrackTextures();
	void AddFallbackTextures();
private:
	size_t maxUniqueScars;

	std::unique_ptr<CTextureAtlas> atlas;
	std::unique_ptr<CTextureRenderAtlas> groundDecalAtlasMain;
	std::unique_ptr<CTextureRenderAtlas> groundDecalAtlasNorm;

	Shader::IProgramObject* decalShader;

	union DecalOwner {
		DecalOwner(const CSolidObject* so_) : so(so_) {};
		DecalOwner(const GhostSolidObject* gso_) : gso(gso_) {};
		const CSolidObject* so;
		const GhostSolidObject* gso;
		const void* ptr;
	};
	struct DecalOwnerHash {
		uint32_t operator()(const auto& s) const {
			static spring::synced_hash<std::uintptr_t> h;
			return h(reinterpret_cast<std::uintptr_t>(s.ptr));
		};
	};
	struct DecalOwnerEqualsTo {
		bool operator()(const DecalOwner& lhs, const DecalOwner& rhs) const
		{
			return reinterpret_cast<std::uintptr_t>(lhs.ptr) == reinterpret_cast<std::uintptr_t>(rhs.ptr);
		}
	};

	spring::unordered_map<DecalOwner, uint32_t, DecalOwnerHash, DecalOwnerEqualsTo> decalOwners; // for tracks, aoplates and ghosts
	std::array<std::pair<float, float>, MAX_UNITS> unitMinMaxHeights; // for tracks

	bool highQuality = false;
	bool tmpNeedsUpdate = false;
	bool permNeedsUpdate = false;

	spring::unordered_map<uint32_t, size_t> decalIdToTmpDecalsVecPos;
	std::vector<bool> tmpUpdateIndicator;

	VBO instTmpVBO;
	VBO instPermVBO;
	VAO vaoTmp;
	VAO vaoPerm;

	CSMFGroundDrawer* smfDrawer;

	static constexpr uint32_t TRACKS_UPDATE_RATE = 4u;
	static constexpr std::pair<float, float> MINMAX_HEIGHT_INIT = std::make_pair(std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest());
};