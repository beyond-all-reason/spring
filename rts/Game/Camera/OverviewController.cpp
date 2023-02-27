/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "OverviewController.h"

#include "Map/Ground.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "System/Log/ILog.h"

static float GetCamHeightToFitMapInView(float mapx, float mapy, float fov) {
    static constexpr float marginForUI = 1.037f; // leave some space for UI outside of map edges
    static constexpr float maxHeight = 25000.0f;
    const float fovCoefficient = 1.0f/math::tan(fov * math::DEG_TO_RAD) * marginForUI;
    return std::min(CGround::GetHeightAboveWater(mapx, mapy, false) +
                    (fovCoefficient * std::max(mapx / globalRendering->aspectRatio, mapy)),
                    maxHeight);
}

COverviewController::COverviewController()
{
	enabled = false;
	minimizeMinimap = false;

	pos.y = GetCamHeightToFitMapInView(pos.x, pos.z, fov/2.0);
	dir = float3(0.0f, -1.0f, -0.001f).ANormalize();
}


float3 COverviewController::SwitchFrom() const
{
	const float3 mdir = mouse->dir;
	const float3 rpos = pos + mdir * CGround::LineGroundCol(pos, pos + mdir * 50000.0f, false);

	if (!globalRendering->dualScreenMode)
		minimap->SetMinimized(minimizeMinimap);

	return rpos;
}

void COverviewController::SwitchTo(const int oldCam, const bool showText)
{
	if (showText)
		LOG("Switching to Overview style camera");

	if (!globalRendering->dualScreenMode) {
		minimizeMinimap = minimap->GetMinimized();
		minimap->SetMinimized(true);
	}
}

void COverviewController::GetState(StateMap& sm) const
{
	CCameraController::GetState(sm);
}

bool COverviewController::SetState(const StateMap& sm)
{
	// CCameraController::SetState(sm);
	// always centered, allow only for FOV change
	SetStateFloat(sm, "fov", fov);
	pos.y = GetCamHeightToFitMapInView(pos.x, pos.z, fov/2.0);
	return true;
}
