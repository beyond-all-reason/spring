/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASIC_MESH_DRAWER_H_
#define _BASIC_MESH_DRAWER_H_

#include "Map/MapDrawPassTypes.h"
#include "Map/SMF/IMeshDrawer.h"
#include "Rendering/GL/RenderBuffersFwd.h"
#include "Rendering/GL/VertexArrayTypes.h"
#include "System/EventClient.h"

#include <array>
#include <memory>
#include <vector>

class CSMFGroundDrawer;
class CCamera;

class CBasicMeshDrawer : public IMeshDrawer, public CEventClient {
public:
	bool GetFullRead() const override { return true; }

	int GetReadAllyTeam() const override { return AllAccessTeam; }

public:
	CBasicMeshDrawer(CSMFGroundDrawer* gd);
	~CBasicMeshDrawer();

	static constexpr int32_t PATCH_SIZE = 128; // must match SMFReadMap::bigSquareSize
	static constexpr int32_t LOD_LEVELS = 8;   // log2(PATCH_SIZE) + 1; 129x129 to 2x2

	enum MAP_BORDERS {
		MAP_BORDER_L = 0,
		MAP_BORDER_R = 1,
		MAP_BORDER_T = 2,
		MAP_BORDER_B = 3,
		MAP_BORDER_C = MAP_BORDER_B + 1
	};

	using MeshRenderBuffer = TypedRenderBuffer<VA_TYPE_0>;
	using BordRenderBuffer = TypedRenderBuffer<VA_TYPE_C>;

	struct MeshVisPatch {
		std::array<uint32_t, 4> visUpdateFrames; // [CAMTYPE_PLAYER -> CAMTYPE_ENVMAP]
	};

	void Update(const DrawPass::e& drawPass);

	void Update() override {}

	void DrawMesh(const DrawPass::e& drawPass) override;
	void DrawBorderMesh(const DrawPass::e& drawPass) override;

private:
	void UploadPatchSquareGeometry(std::unique_ptr<MeshRenderBuffer>& meshRenderBuffer, uint32_t lodStep);
	void
	UploadPatchBorderGeometry(std::unique_ptr<BordRenderBuffer>& borderRenderBuffer, MAP_BORDERS b, uint32_t lodStep);

	void DrawSquareMeshPatch() const;
	void DrawBorderMeshPatch(const CCamera* activeCam, uint32_t borderSide) const;

	uint32_t CalcDrawPassLOD(const CCamera* cam, const DrawPass::e& drawPass) const;

private:
	uint32_t numPatchesX;
	uint32_t numPatchesY;
	uint32_t drawPassLOD;

	std::vector<MeshVisPatch> meshVisPatches;
	std::array<std::unique_ptr<MeshRenderBuffer>, LOD_LEVELS> meshRenderBuffers;
	std::array<std::unique_ptr<BordRenderBuffer>, MAP_BORDERS::MAP_BORDER_C * LOD_LEVELS> borderRenderBuffers;

	std::array<float, LOD_LEVELS> lodDistTable;

	CSMFGroundDrawer* smfGroundDrawer;
};

#endif
