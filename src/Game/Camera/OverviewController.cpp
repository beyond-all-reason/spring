/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "OverviewController.h"

#include "Map/Ground.h"
#include "Game/UI/MiniMap.h"
#include "Game/UI/MouseHandler.h"
#include "System/Log/ILog.h"
#include "Game/Camera.h"
#include "System/Config/ConfigHandler.h"

#include "System/Misc/TracyDefs.h"

CONFIG(bool, CamOverviewDynamicRotation).defaultValue(false).description("Transition from different camera preserves rotation");

static float GetCamHeightToFitMapInView(float mapx, float mapy, float fov, bool cameraSideways) {
	RECOIL_DETAILED_TRACY_ZONE;
	static constexpr float marginForUI = 1.037f; // leave some space for UI outside of map edges
	static constexpr float maxHeight = 25000.0f;
	const float fovCoefficient = 1.0f/math::tan(fov * math::DEG_TO_RAD) * marginForUI;
	const float aspectRatio = cameraSideways ?
		std::max(mapy / globalRendering->aspectRatio, mapx) :
		std::max(mapx / globalRendering->aspectRatio, mapy);
    return std::min(CGround::GetHeightAboveWater(mapx, mapy, false) +
			fovCoefficient * aspectRatio,
			maxHeight);
}

static float GetClosestRightAngle(float angle) {
	RECOIL_DETAILED_TRACY_ZONE;
	return std::round(angle/math::HALFPI) * math::HALFPI;
}

static bool CameraPointingSideways(float angle) {
	RECOIL_DETAILED_TRACY_ZONE;
	return std::lround(angle/math::HALFPI) % 2;
}

COverviewController::COverviewController() :
	minimizeMinimap(false),
	dynamicRotation(false),
	camRotY(CCameraController::GetRot().y)
{
	RECOIL_DETAILED_TRACY_ZONE;
	enabled = false;

	dir = float3(0.0f, -1.0f, -0.001f).ANormalize();

	bool cameraSideways = CameraPointingSideways(GetRot().y);
	pos.y = GetCamHeightToFitMapInView(pos.x, pos.z, fov/2.0, cameraSideways);

	configHandler->NotifyOnChange(this, {"CamOverviewDynamicRotation"});
	ConfigUpdate();
}

COverviewController::~COverviewController()
{
	RECOIL_DETAILED_TRACY_ZONE;
	configHandler->RemoveObserver(this);
}

float3 COverviewController::SwitchFrom() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 mdir = mouse->dir;
	const float3 rpos = pos + mdir * CGround::LineGroundCol(pos, pos + mdir * 50000.0f, false);

	if (!globalRendering->dualScreenMode)
		minimap->SetMinimized(minimizeMinimap);

	return rpos;
}

float3 COverviewController::GetRot() const {
	RECOIL_DETAILED_TRACY_ZONE;
	const float3 defaultRot = CCameraController::GetRot();
	if (!this->dynamicRotation)
	{
		return defaultRot;
	}

	return {defaultRot.x, GetClosestRightAngle(camRotY), defaultRot.z};
}

void COverviewController::SwitchTo(const CCameraController* oldCam, const bool showText)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (showText)
		LOG("Switching to Overview style camera");

	if (!globalRendering->dualScreenMode) {
		minimizeMinimap = minimap->GetMinimized();
		minimap->SetMinimized(true);
	}

	camRotY = oldCam->GetRot().y;

	bool cameraSideways = CameraPointingSideways(oldCam->GetRot().y);
	pos.y = GetCamHeightToFitMapInView(pos.x, pos.z, fov/2.0, cameraSideways);
}

void COverviewController::GetState(StateMap& sm) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::GetState(sm);
}

bool COverviewController::SetState(const StateMap& sm)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// CCameraController::SetState(sm);
	// always centered, allow only for FOV change
	SetStateFloat(sm, "fov", fov);
	return true;
}

void COverviewController::ConfigUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	dynamicRotation = configHandler->GetBool("CamOverviewDynamicRotation");
}

void COverviewController::ConfigNotify(const std::string & key, const std::string & value)
{
	RECOIL_DETAILED_TRACY_ZONE;
	ConfigUpdate();
}
