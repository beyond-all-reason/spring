/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keycode.h>

#include "DollyController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Misc/SpringTime.h"
#include "System/SpringMath.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/ModInfo.h"
#include "System/Math/NURBS.h"
#include "System/Misc/TracyDefs.h"
#include "System/float3.h"

namespace {
	enum HeightTracking : int {
		Disabled = 0,
		Terrain,
		Smooth,
	};
}

CONFIG(bool,  CamDollyEnabled).defaultValue(true).headlessValue(false);
CONFIG(int,   CamDollyScrollSpeed).defaultValue(10);
CONFIG(float, CamDollyFOV).defaultValue(45.0f);
CONFIG(bool,  CamDollyLockCardinalDirections).defaultValue(true).description("Whether cardinal directions should be `locked` for a short time when rotating.");
CONFIG(bool,  CamDollyZoomInToMousePos).defaultValue(true);
CONFIG(bool,  CamDollyZoomOutFromMousePos).defaultValue(false);
CONFIG(bool,  CamDollyEdgeRotate).defaultValue(false).description("Rotate camera when cursor touches screen borders.");
CONFIG(float, CamDollyFastScaleMouseMove).defaultValue(3.0f / 10.0f).description("Scaling for CameraMoveFastMult in spring camera mode while moving mouse.");
CONFIG(float, CamDollyFastScaleMousewheelMove).defaultValue(2.0f / 10.0f).description("Scaling for CameraMoveFastMult in spring camera mode while scrolling with mouse.");
CONFIG(int,   CamDollyTrackMapHeightMode).defaultValue(HeightTracking::Terrain).description("Camera height is influenced by terrain height. 0=Static 1=Terrain 2=Smoothmesh");

static float DistanceToGround(float3 from, float3 dir, float fallbackPlaneHeight) {
	RECOIL_DETAILED_TRACY_ZONE;
	float newGroundDist = CGround::LineGroundCol(from, from + dir * 150000.0f, false);

	// if the direction is not pointing towards the map we use provided xz plane as heuristic
	if (newGroundDist <= 0.0f)
		newGroundDist = CGround::LinePlaneCol(from, dir, 150000.0f, fallbackPlaneHeight);

	return newGroundDist;
}

CDollyController::CDollyController()
	: rot(2.677f, 0.0f, 0.0f)
	, curDist(float3(mapDims.mapx * 0.5f, 0.0f, mapDims.mapy * 0.55f).Length2D() * 1.5f * SQUARE_SIZE)
	, maxDist(std::max(mapDims.mapx, mapDims.mapy) * SQUARE_SIZE * 1.333f)
	, oldDist(0.0f)
	, zoomBack(false),
	lookTarget(float3(5000.f,100.f,5000.f))
{
	RECOIL_DETAILED_TRACY_ZONE;
	enabled = configHandler->GetBool("CamDollyEnabled");
	configHandler->NotifyOnChange(this, {"CamDollyScrollSpeed", "CamDollyFOV", "CamDollyZoomInToMousePos",
		"CamDollyZoomOutFromMousePos", "CamDollyFastScaleMousewheelMove", "CamDollyFastScaleMouseMove", "CamDollyEdgeRotate",
		"CamDollyLockCardinalDirections", "CamDollyTrackMapHeightMode"});
	ConfigUpdate();
}

CDollyController::~CDollyController()
{
	RECOIL_DETAILED_TRACY_ZONE;
	configHandler->RemoveObserver(this);
}


void CDollyController::ConfigUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	scrollSpeed = configHandler->GetFloat("CamDollyScrollSpeed") * 0.1f;
	fov = configHandler->GetFloat("CamDollyFOV");
	cursorZoomIn = configHandler->GetBool("CamDollyZoomInToMousePos");
	cursorZoomOut = configHandler->GetBool("CamDollyZoomOutFromMousePos");
	fastScaleMove = configHandler->GetFloat("CamDollyFastScaleMouseMove");
	fastScaleMousewheel = configHandler->GetFloat("CamDollyFastScaleMousewheelMove");
	doRotate = configHandler->GetBool("CamDollyEdgeRotate");
	lockCardinalDirections = configHandler->GetBool("CamDollyLockCardinalDirections");
	trackMapHeight = configHandler->GetInt("CamDollyTrackMapHeightMode");

	if (trackMapHeight == HeightTracking::Smooth && !modInfo.enableSmoothMesh) {
		LOG_L(L_ERROR, "Smooth mesh disabled");
		trackMapHeight = HeightTracking::Terrain;
	}
}

void CDollyController::ConfigNotify(const std::string & key, const std::string & value)
{
	RECOIL_DETAILED_TRACY_ZONE;
	ConfigUpdate();
}

void CDollyController::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	float curTime = spring_gettime().toMilliSecsf();
	float percent = std::clamp(1 - (endTime - curTime) / (endTime - startTime), 0.f, 1.f);
	float minU = 0.f;
	float maxU = 1.f;
	float u = minU + percent * (maxU - minU);
	pos = SolveNURBS(curveDegree, curveControlPoints, nurbsKnots, u);
	// LOG_L(L_INFO, "Dollypos: %s", pos.str().c_str());


	// pos.ClampInMap();
	// pos.y = std::min(pos.y, CGround::GetHeightReal(pos.x, pos.z, false)); // always focus on the ground
	dir =(lookTarget - pos).Normalize();
	float3 newRot = CCamera::GetRotFromDir(GetDir());
	rot = rot + GetRadAngleToward(rot, newRot);
	camHandler->CameraTransition(0.f);
	//rot.x = std::clamp(rot.x, math::PI * 0.51f, math::PI * 0.99f);

	// camera->SetRot(float3(rot.x, GetAzimuth(), rot.z));
	//dir = CCamera::GetFwdFromRot(this->GetRot());

	//curDist = std::clamp(curDist, 20.0f, maxDist);
	//pixelSize = (camera->GetTanHalfFov() * 2.0f) / globalRendering->viewSizeY * curDist * 2.0f;
	// camera->SetPos(pos);
		// camera->SetRot(rot);
}


static float GetRotationWithCardinalLock(float rot)
{
	RECOIL_DETAILED_TRACY_ZONE;
	constexpr float cardinalDirLockWidth = 0.2f;

	const float rotMoved = std::abs(rot /= math::HALFPI) - cardinalDirLockWidth * 0.5f;
	const float numerator = std::trunc(rotMoved);

	const float fract = rotMoved - numerator;
	const float b = 1.0f / (1.0f - cardinalDirLockWidth);
	const float c = 1.0f - b;
	const float fx = (fract > cardinalDirLockWidth) ? fract * b + c : 0.0f;

	return std::copysign(numerator + fx, rot) * math::HALFPI;
}

void CDollyController::SwitchTo(const int oldCam, const bool showText)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (showText)
		LOG("Switching to Dolly style camera");

	if (oldCam == CCameraHandler::CAMERA_MODE_OVERVIEW)
		return;
	curveControlPoints = {
			{-3595.333, 9152.273, 15470.391, 1.0},
			{1331.407, 3582.310, 10525.238, 1.0},
			{3426.812, 833.980, 7029.465, 1.0},
			{3334.951, 909.047, 3736.86, 1.0},
			{8099.745, 1582.447, 55.414, 1.0}
	};
	nurbsKnots = {0.0, 0.0, 0.0, 0.0, 0.5,1.0, 1.0, 1.0, 1.0};
	startTime = spring_gettime().toMilliSecsf();
	endTime = startTime + 10000;
	lookTarget = float3(7530.750, 152.520, 3352.290);
}


void CDollyController::GetState(StateMap& sm) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::GetState(sm);
	sm["dist"] = curDist;
	sm["rx"]   = rot.x;
	sm["ry"]   = rot.y;
	sm["rz"]   = rot.z;
}


bool CDollyController::SetState(const StateMap& sm)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::SetState(sm);
	SetStateFloat(sm, "dist", curDist);
	SetStateFloat(sm, "rx",   rot.x);
	SetStateFloat(sm, "ry",   rot.y);
	SetStateFloat(sm, "rz",   rot.z);
	return true;
}
