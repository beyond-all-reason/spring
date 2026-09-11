/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ROAM_MESH_DRAWER_H_
#define _ROAM_MESH_DRAWER_H_

#include <vector>

#include "Patch.h"
#include "Map/BaseGroundDrawer.h"
#include "Map/SMF/IMeshDrawer.h"
#include "System/EventHandler.h"



class CSMFGroundDrawer;
class CCamera;



// Visualize visible patches in Minimap for debugging?
// #define DRAW_DEBUG_IN_MINIMAP


/**
 * Map mesh drawer implementation; based on the Tread Marks engine
 * by Longbow Digital Arts (www.LongbowDigitalArts.com, circa 2000)
 */
class CRoamMeshDrawer : public IMeshDrawer, public CEventClient
{
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return (eventName == "UnsyncedHeightMapUpdate") || (eventName == "DrawInMiniMap");
	}
	bool GetFullRead() const { return true; }
	int  GetReadAllyTeam() const { return AllAccessTeam; }

	void UnsyncedHeightMapUpdate(const SRectangle& rect);
	void DrawInMiniMap();

public:
	enum {
		MESH_NORMAL = 0,
		MESH_SHADOW = 1,
		MESH_COUNT  = 2,
	};

	CRoamMeshDrawer(CSMFGroundDrawer* gd);
	~CRoamMeshDrawer();

	void Update();

	void DrawMesh(const DrawPass::e& drawPass);
	void DrawBorderMesh(const DrawPass::e& drawPass);

	static void ForceNextTesselation(bool normal, bool shadow) {
		forceNextTesselation[MESH_NORMAL] = normal;
		forceNextTesselation[MESH_SHADOW] = shadow;
	}

private:
	void Reset(bool shadowPass);
	void Tessellate(std::vector<Patch>& patches, const CCamera* cam, int viewRadius, bool shadowPass);

	// batched shadow pass (see shadowIndxVBO)
	void InitShadowVAO();
	bool AllocShadowIndexRegion(int patchIdx, uint32_t numIndices);
	void RepackShadowIndices(const std::vector<Patch>& patches);
	int UploadShadowPatches(const CCamera* cam, std::vector<Patch>& patches);
	void DrawShadowMeshBatched(const CCamera* cam);

private:
	struct ShadowIndexRegion {
		uint32_t offset = 0;   // in indices
		uint32_t capacity = 0; // in indices
	};

	CSMFGroundDrawer* smfGroundDrawer;

	// the patch-local vertex grid is the same for every patch; one buffer serves all of them
	VBO patchVertVBO;

	// Shadow-pass patches are all drawn with a single glMultiDrawElementsIndirect:
	// their index-lists live in regions of one shared element buffer (regions are
	// (re)allocated as tessellation changes, with the whole buffer repacked when
	// the tail runs out or too much of it is garbage) and each draw's baseInstance
	// selects the patch's square from shadowSquareVBO via an instanced attribute.
	// Falls back to one draw per patch if the required GL features are missing.
	bool batchedShadowPass = false;

	VBO shadowIndxVBO;
	VBO shadowSquareVBO;
	VAO shadowVAO;

	std::vector<ShadowIndexRegion> shadowIndexRegions;
	std::vector<SDrawElementsIndirectCommand> shadowDrawCmds;

	uint32_t shadowIndxUsed = 0;     // regions are packed in [0, used)
	uint32_t shadowIndxCapacity = 0; // size of shadowIndxVBO, in indices
	uint32_t shadowIndxGarbage = 0;  // abandoned region space inside [0, used)

	int numPatchesX = 0;
	int numPatchesY = 0;
	std::array<int, MESH_COUNT> lastGroundDetail = {};

	bool heightMapChanged = false;

	std::array<float3, MESH_COUNT> lastCamPos;
	std::array<float3, MESH_COUNT> lastCamDir;

	std::array<int, MESH_COUNT> numPatchesLeftVisibility = {};
	std::array<int, MESH_COUNT> tesselationsSinceLastReset = {};
	std::function<bool(std::vector<Patch>&, const CCamera*, int, bool)> tesselateFuncs[2];

	// [1] is used for the shadow pass, [0] is used for all other passes
	std::vector< Patch > patchMeshGrid[MESH_COUNT];
	std::vector< Patch*> borderPatches[MESH_COUNT];

	// char instead of bool, accessors to different elements must be thread-safe
	std::vector<uint8_t> patchVisFlags[MESH_COUNT];

	// whether tessellation should be forcibly performed next frame
	static bool forceNextTesselation[MESH_COUNT];

#ifdef DRAW_DEBUG_IN_MINIMAP
	std::vector<float3> debugColors;
#endif
};

#endif // _ROAM_MESH_DRAWER_H_
