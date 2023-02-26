/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "OverviewController.h"

#include "Map/Ground.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "System/Log/ILog.h"

static float fit_cam_height_to_map(float map_x, float map_y, float fov) {
    const auto extra_margin = 1.037;
    const auto fov_coefficient = 1.0/math::tan(fov * math::DEG_TO_RAD) * extra_margin;
    const auto max_height = 25000.0;
    return std::min(CGround::GetHeightAboveWater(map_x, map_y, false) +
                    (fov_coefficient * std::max(map_x / globalRendering->aspectRatio, map_y)),
                    max_height);
}

COverviewController::COverviewController()
{
	enabled = false;
	minimizeMinimap = false;

	pos.y = fit_cam_height_to_map(pos.x, pos.z, fov/2.0);
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
	pos.y = fit_cam_height_to_map(pos.x, pos.z, fov/2.0);
	return true;
}
