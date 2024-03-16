/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BasicMeshDrawer.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Map/SMF/SMFGroundDrawer.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/RenderBuffers.h"
#include "System/EventHandler.h"

#include <tracy/Tracy.hpp>


CBasicMeshDrawer::CBasicMeshDrawer(CSMFGroundDrawer* gd)
	: CEventClient("[CBasicMeshDrawer]", 717171, false)
	, smfGroundDrawer(gd)
{
	eventHandler.AddClient(this);

	numPatchesX = mapDims.mapx / PATCH_SIZE;
	numPatchesY = mapDims.mapy / PATCH_SIZE;
	drawPassLOD = 0;

	assert(numPatchesX >= 1);
	assert(numPatchesY >= 1);

	// const auto& lodDistFunc = [](uint32_t i) { return (125.0f * i * i        ); }; // f(x)=125.0*x^2
	// const auto& lodDistFunc = [](uint32_t i) { return ( 25.0f * i * i * i    ); }; // f(x)=25.0*x^3
	const auto& lodDistFunc = [](uint32_t i) { return (  5.0f * i * i * i * i); }; // f(x)=5.0*x^4

	for (uint32_t n = 0; n < LOD_LEVELS; n++) {
		lodDistTable[n] = lodDistFunc(n + 1);
	}

	uint32_t lod = 0;
	for (auto& meshRenderBuffer : meshRenderBuffers) {
		const uint32_t lodStep = 1 << lod;
		const size_t numVert = Square(PATCH_SIZE / lodStep + 1);
		const size_t numIndx = Square(PATCH_SIZE / lodStep    ) * 6;
		meshRenderBuffer = std::make_unique<MeshRenderBuffer>(numVert, numIndx, IStreamBufferConcept::SB_BUFFERSUBDATA, false);
		UploadPatchSquareGeometry(meshRenderBuffer, lodStep);
		meshRenderBuffer->SetReadonly();
		lod++;
	}

	for (uint32_t lod = 0; lod < LOD_LEVELS; lod++) {
		const uint32_t lodStep = 1 << lod;
		const size_t numVert = (PATCH_SIZE / lodStep + 1) * 2;
		const size_t numIndx = (PATCH_SIZE / lodStep    ) * 6;
		for (uint32_t b = MAP_BORDER_L; b < MAP_BORDER_C; b++) {
			auto& borderRenderBuffer = borderRenderBuffers[lod * static_cast<uint32_t>(MAP_BORDER_C) + static_cast<uint32_t>(b)];
			borderRenderBuffer = std::make_unique<BordRenderBuffer>(numVert, numIndx, IStreamBufferConcept::SB_BUFFERSUBDATA, false);
			UploadPatchBorderGeometry(borderRenderBuffer, static_cast<MAP_BORDERS>(b), lodStep);
			borderRenderBuffer->SetReadonly();
		}
	}

	meshVisPatches.resize(numPatchesX * numPatchesY);
	for (auto& meshVisPatch : meshVisPatches) {
		meshVisPatch.visUpdateFrames.fill(0);
	}
}

CBasicMeshDrawer::~CBasicMeshDrawer()
{
	//ZoneScoped;
	eventHandler.RemoveClient(this);

	meshRenderBuffers = {};
	borderRenderBuffers = {};
}



void CBasicMeshDrawer::Update(const DrawPass::e& drawPass)
{
	//ZoneScoped;
	CCamera* activeCam = CCameraHandler::GetActiveCamera();

	static constexpr float wsEdge = PATCH_SIZE * SQUARE_SIZE;

	const int drawQuadsX = mapDims.mapx / PATCH_SIZE;
	const int drawQuadsZ = mapDims.mapy / PATCH_SIZE;

	for (int x = 0; x < drawQuadsX; ++x) {
		for (int z = 0; z < drawQuadsZ; ++z) {
			const auto& uhmi = readMap->GetUnsyncedHeightInfo(x, z);

			AABB aabb {
				{ (x + 0) * wsEdge, uhmi.x, (z + 0) * wsEdge },
				{ (x + 1) * wsEdge, uhmi.y, (z + 1) * wsEdge }
			};

			if (!activeCam->InView(aabb))
				continue;

			meshVisPatches[z * numPatchesX + x].visUpdateFrames[activeCam->GetCamType()] = globalRendering->drawFrame;
		}
	}

	drawPassLOD = CalcDrawPassLOD(activeCam, drawPass);
}

void CBasicMeshDrawer::UploadPatchSquareGeometry(std::unique_ptr<MeshRenderBuffer>& meshRenderBuffer, uint32_t lodStep)
{
	//ZoneScoped;
	meshRenderBuffer->MakeQuadsTriangles(
		{ { 0.0f                    , 0.0f ,       0.0f               } },
		{ { PATCH_SIZE * SQUARE_SIZE, 0.0f ,       0.0f               } },
		{ { PATCH_SIZE * SQUARE_SIZE, 0.0f , PATCH_SIZE * SQUARE_SIZE } },
		{ { 0.0f                    , 0.0f , PATCH_SIZE * SQUARE_SIZE } },
		PATCH_SIZE / lodStep,
		PATCH_SIZE / lodStep
	);
}

void CBasicMeshDrawer::UploadPatchBorderGeometry(std::unique_ptr<BordRenderBuffer>& borderRenderBuffer, MAP_BORDERS b, uint32_t lodStep)
{
	//ZoneScoped;
	auto tl = VA_TYPE_C{ {0.0f,  0.0f ,0.0f}, { 255, 255, 255, 255 } };
	auto tr = VA_TYPE_C{ {0.0f,  0.0f ,0.0f}, { 255, 255, 255, 255 } };
	auto bl = VA_TYPE_C{ {0.0f, -1.0f ,0.0f}, { 255, 255, 255,   0 } };
	auto br = VA_TYPE_C{ {0.0f, -1.0f ,0.0f}, { 255, 255, 255,   0 } };

	switch (b)
	{
	case CBasicMeshDrawer::MAP_BORDER_L: {
		tl.pos.x = 0.0f; tl.pos.z = 0.0f;
		bl.pos.x = 0.0f; bl.pos.z = 0.0f;
		tr.pos.x = 0.0f; tr.pos.z = PATCH_SIZE * SQUARE_SIZE;
		br.pos.x = 0.0f; br.pos.z = PATCH_SIZE * SQUARE_SIZE;
	} break;
	case CBasicMeshDrawer::MAP_BORDER_R: {
		tl.pos.x = PATCH_SIZE * SQUARE_SIZE; tl.pos.z = PATCH_SIZE * SQUARE_SIZE;
		bl.pos.x = PATCH_SIZE * SQUARE_SIZE; bl.pos.z = PATCH_SIZE * SQUARE_SIZE;
		tr.pos.x = PATCH_SIZE * SQUARE_SIZE; tr.pos.z = 0.0f;
		br.pos.x = PATCH_SIZE * SQUARE_SIZE; br.pos.z = 0.0f;
	} break;
	case CBasicMeshDrawer::MAP_BORDER_T: {
		tl.pos.x = PATCH_SIZE * SQUARE_SIZE; tl.pos.z = 0.0f;
		bl.pos.x = PATCH_SIZE * SQUARE_SIZE; bl.pos.z = 0.0f;
		tr.pos.x = 0.0f;                     tr.pos.z = 0.0f;
		br.pos.x = 0.0f;                     br.pos.z = 0.0f;
	} break;
	case CBasicMeshDrawer::MAP_BORDER_B: {
		tl.pos.x = 0.0f;                     tl.pos.z = PATCH_SIZE * SQUARE_SIZE;
		bl.pos.x = 0.0f;                     bl.pos.z = PATCH_SIZE * SQUARE_SIZE;
		tr.pos.x = PATCH_SIZE * SQUARE_SIZE; tr.pos.z = PATCH_SIZE * SQUARE_SIZE;
		br.pos.x = PATCH_SIZE * SQUARE_SIZE; br.pos.z = PATCH_SIZE * SQUARE_SIZE;
	} break;
	default:
		assert(false);
		break;
	}

	borderRenderBuffer->MakeQuadsTriangles(
		tl,
		tr,
		br,
		bl,
		PATCH_SIZE / lodStep,
		1
	);
}



uint32_t CBasicMeshDrawer::CalcDrawPassLOD(const CCamera* cam, const DrawPass::e& drawPass) const
{
	//ZoneScoped;
	// higher detail biases LOD-step toward a smaller value
	// NOTE: should perhaps prevent an insane initial bias?
	int32_t lodBias = smfGroundDrawer->GetGroundDetail(drawPass) % LOD_LEVELS;
	int32_t lodIndx = LOD_LEVELS - 1;

	// force SP and NP to equal LOD; avoids projection issues
	if (drawPass == DrawPass::Shadow || drawPass == DrawPass::WaterReflection)
		cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	{
		const CUnit* hitUnit = nullptr;
		const CFeature* hitFeature = nullptr;

		float mapRayDist = 0.0f;

		if ((mapRayDist = TraceRay::GuiTraceRay(cam->GetPos(), cam->GetDir(), cam->GetFarPlaneDist(), nullptr, hitUnit, hitFeature, false, true, true)) < 0.0f)
			mapRayDist = CGround::LinePlaneCol(cam->GetPos(), cam->GetDir(), cam->GetFarPlaneDist(), readMap->GetCurrMinHeight());
		if (mapRayDist < 0.0f)
			return lodIndx;

		for (uint32_t n = 0; n < LOD_LEVELS; n += 1) {
			if (mapRayDist < lodDistTable[n]) {
				lodIndx = n;
				break;
			}
		}
	}

	switch (drawPass) {
		case DrawPass::Normal         : { return (std::max(lodIndx - lodBias, 0)); } break;
		case DrawPass::Shadow         : { return (std::max(lodIndx - lodBias, 0)); } break;
		case DrawPass::TerrainDeferred: { return (std::max(lodIndx - lodBias, 0)); } break;
		default: {} break;
	}

	// prevent reflections etc from becoming too low-res
	return (std::clamp(lodIndx - lodBias, 0, LOD_LEVELS - 4));
}



void CBasicMeshDrawer::DrawSquareMeshPatch() const
{
	//ZoneScoped;
	meshRenderBuffers[drawPassLOD]->DrawElements(GL_TRIANGLES, false);
}

void CBasicMeshDrawer::DrawMesh(const DrawPass::e& drawPass)
{
	//ZoneScoped;
	Update(drawPass);

	const CCamera* activeCam = CCameraHandler::GetActiveCamera();

	for (uint32_t py = 0; py < numPatchesY; py += 1) {
		for (uint32_t px = 0; px < numPatchesX; px += 1) {
			const auto& meshVisPatch = meshVisPatches[py * numPatchesX + px];

			if (meshVisPatch.visUpdateFrames[activeCam->GetCamType()] < globalRendering->drawFrame)
				continue;

			smfGroundDrawer->SetupBigSquare(drawPass, px, py);

			DrawSquareMeshPatch();
		}
	}
}



void CBasicMeshDrawer::DrawBorderMeshPatch(const CCamera* activeCam, uint32_t borderSide) const
{
	//ZoneScoped;
	const auto idx = drawPassLOD * static_cast<uint32_t>(MAP_BORDER_C) + static_cast<uint32_t>(borderSide);
	borderRenderBuffers[idx]->DrawElements(GL_TRIANGLES, false);
}

void CBasicMeshDrawer::DrawBorderMesh(const DrawPass::e& drawPass)
{
	//ZoneScoped;
	const uint32_t npxm1 = numPatchesX - 1;
	const uint32_t npym1 = numPatchesY - 1;

	const CCamera* activeCam  = CCameraHandler::GetActiveCamera();
	const uint32_t actCamType = activeCam->GetCamType();

	//glFrontFace(GL_CW);
	for (uint32_t px = 0; px < numPatchesX; px++) {
		if (meshVisPatches[0 * numPatchesX + px].visUpdateFrames[actCamType] < globalRendering->drawFrame)
			continue;

		smfGroundDrawer->SetupBigSquare(drawPass, px, 0);
		DrawBorderMeshPatch(activeCam, MAP_BORDER_T);
	}
	for (uint32_t py = 0; py < numPatchesY; py++) {
		if (meshVisPatches[py * numPatchesX + npxm1].visUpdateFrames[actCamType] < globalRendering->drawFrame)
			continue;

		smfGroundDrawer->SetupBigSquare(drawPass, npxm1, py);
		DrawBorderMeshPatch(activeCam, MAP_BORDER_R);
	}

	//glFrontFace(GL_CCW);
	for (uint32_t px = 0; px < numPatchesX; px++) {
		if (meshVisPatches[npym1 * numPatchesX + px].visUpdateFrames[actCamType] < globalRendering->drawFrame)
			continue;

		smfGroundDrawer->SetupBigSquare(drawPass, px, npym1);
		DrawBorderMeshPatch(activeCam, MAP_BORDER_B);
	}
	for (uint32_t py = 0; py < numPatchesY; py++) {
		if (meshVisPatches[py * numPatchesX + 0].visUpdateFrames[actCamType] < globalRendering->drawFrame)
			continue;

		smfGroundDrawer->SetupBigSquare(drawPass, 0, py);
		DrawBorderMeshPatch(activeCam, MAP_BORDER_L);
	}
}
