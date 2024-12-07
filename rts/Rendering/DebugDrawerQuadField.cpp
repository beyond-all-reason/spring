/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DebugDrawerQuadField.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Game/SelectedUnitsHandler.h"
#include "Game/UI/MouseHandler.h"

#include "Map/Ground.h"
#include "Map/ReadMap.h"

#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/UnitHandler.h"

#include "Rendering/LineDrawer.h"

#include "System/EventHandler.h"
#include "System/GlobalConfig.h"
#include "System/Misc/TracyDefs.h"
#include "System/SafeUtil.h"


DebugDrawerQuadField* DebugDrawerQuadField::instance = nullptr;

DebugDrawerQuadField::DebugDrawerQuadField()
: CEventClient("[DebugDrawerQuadField]", 199991, false)
{
	autoLinkEvents = true;
	RegisterLinkedEvents(this);
	eventHandler.AddClient(this);
}

void DebugDrawerQuadField::SetEnabled(bool enable)
{
	if (!enable) {
		spring::SafeDelete(instance);
		return;
	}

	assert(instance == nullptr);
	instance = new DebugDrawerQuadField();
}

void DebugDrawerQuadField::DrawWorldPreUnit()
{
	// lineDrawer cleans up after drawing, so we need to queue lines
	// before minimap and world drawing.
	DrawAll();
}

void DebugDrawerQuadField::DrawInMiniMapBackground()
{
	DrawAll();
}

void DebugDrawerQuadField::DrawAll() const
{
	RECOIL_DETAILED_TRACY_ZONE;

	DrawSelectionQuads();
	DrawMouseRayQuads();
	DrawCamera();
}

void DebugDrawerQuadField::DrawRect(float3 pos, float widthX, float widthZ, float4 color) const
{
	const float x0 = pos.x - widthX/2.0;
	const float x1 = pos.x + widthX/2.0;
	const float y0 = pos.z - widthZ/2.0;
	const float y1 = pos.z + widthZ/2.0;

	const float h = CGround::GetHeightReal(pos.x, pos.z, false);

	lineDrawer.StartPath(float3(x0, h, y0), color);
	lineDrawer.DrawLine(float3(x1, h, y0), color);
	lineDrawer.DrawLine(float3(x1, h, y1), color);
	lineDrawer.DrawLine(float3(x0, h, y1), color);
	lineDrawer.DrawLine(float3(x0, h, y0), color);
	lineDrawer.FinishPath();
}

void DebugDrawerQuadField::DrawQuad(unsigned i, float4 color) const
{
	const int qx = i % quadField.GetNumQuadsX();
	const int qz = i / quadField.GetNumQuadsZ();

	const int quadSizeX = quadField.GetQuadSizeX();
	const int quadSizeZ = quadField.GetQuadSizeZ();

	const float3 pos = float3((qx+0.5)*quadSizeX, 0, (qz+0.5)*quadSizeZ);

	DrawRect(pos, 1.0*quadSizeX, 1.0*quadSizeZ, color);
}

void DebugDrawerQuadField::DrawSelectionQuads() const
{
	const float4 quadColor = {0.4, 1.0, 0.4, 1.0};
	const auto& selectedUnits = selectedUnitsHandler.selectedUnits;
	for (const int unitID: selectedUnits) {
		const CUnit* su = unitHandler.GetUnit(unitID);
		for(const int qIdx: su->quads) {
			DrawQuad(qIdx, quadColor);
		}
	}
}

void DebugDrawerQuadField::DrawMouseRayQuads() const
{
	// Replicating GuiTraceRay ray setup and query so we can draw the same
	const float3 start = camera->GetPos();
	const float3 dir = mouse->dir;
	const float length = camera->GetFarPlaneDist() * 1.4f;

	const float    guiRayLength = length;
	const float groundRayLength = CGround::LineGroundCol(start, dir, length, false);
	const float  waterRayLength = CGround::LinePlaneCol(start, dir, length, CGround::GetWaterPlaneLevel());

	float maxRayLength;
	if (groundRayLength >= 0.0) {
		maxRayLength = groundRayLength;
	} else if (waterRayLength >= 0.0) {
		maxRayLength = waterRayLength;
	} else {
		maxRayLength = length;
	}
	maxRayLength = std::min(maxRayLength + globalConfig.selectThroughGround, length);

	QuadFieldQuery qfQuery;
	const float allyTeamError = losHandler->GetAllyTeamRadarErrorSize(gu->myAllyTeam);
	quadField.GetQuadsOnWideRay(qfQuery, start, dir, maxRayLength, allyTeamError);

	for (const int quadIdx: *qfQuery.quads) {
		DrawQuad(quadIdx, float4(1.0, 1.0, 1.0, 1.0));
	}
}

float3 DebugDrawerQuadField::TraceToMaxHeight(float3 start, float3 point, float length) const
{
	// return intersection with max map height or just original start if we don't find any
	const float3 dir = (point-start).Normalize();
	const float maxHeight = readMap->GetCurrMaxHeight();

	const float dist = CGround::LinePlaneCol(start, dir, length, maxHeight);
	if (dist < 0.0)
		return start;

	return start + dir * dist;
}

void DebugDrawerQuadField::DrawCamera() const
{
	// draw some camera information we can use to find a better ray start.
	const float3 start = camera->GetPos();
	const float3 dir = mouse->dir;
	const float length = camera->GetFarPlaneDist() * 1.4f;

	constexpr float w = 100;
	constexpr auto camColor   = float4(1.0, 1.0, 1.0, 0.8);
	constexpr auto frustColor = float4(1.0, 0.5, 0.5, 1.0);

	// draw camera position
	DrawRect(start, w, w, camColor);

	// intersection of frustum to map max height
	for(int i=0; i<4; i++) {
		auto fv = camera->GetFrustumVert(CCamera::FRUSTUM_POINT_FBL+i);
		DrawRect(TraceToMaxHeight(start, fv, length), w, w, camColor);
	}

	// draw intersection of near frustum to mouse ray
	// note: seems to be the same as camPos
	float3 intersection;
	float4 nearPlane = camera->GetFrustumPlane(CCamera::FRUSTUM_PLANE_NEA);
	bool res = RayAndPlaneIntersection(start, start + dir * length, nearPlane, false, intersection);
	if (res)
		DrawRect(intersection, w*0.9, w*0.9, frustColor);
}

