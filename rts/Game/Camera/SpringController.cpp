/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keycode.h>

#include "SpringController.h"
#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Game/UI/MouseHandler.h"
#include "Rendering/GlobalRendering.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"
#include "System/Input/KeyInput.h"
#include "Sim/Misc/SmoothHeightMesh.h"
#include "Sim/Misc/ModInfo.h"

#include <tracy/Tracy.hpp>

namespace {
	enum HeightTracking : int {
		Disabled = 0,
		Terrain,
		Smooth,
	};
}

CONFIG(bool,  CamSpringEnabled).defaultValue(true).headlessValue(false);
CONFIG(int,   CamSpringScrollSpeed).defaultValue(10);
CONFIG(float, CamSpringFOV).defaultValue(45.0f);
CONFIG(bool,  CamSpringLockCardinalDirections).defaultValue(true).description("Whether cardinal directions should be `locked` for a short time when rotating.");
CONFIG(bool,  CamSpringZoomInToMousePos).defaultValue(true);
CONFIG(bool,  CamSpringZoomOutFromMousePos).defaultValue(false);
CONFIG(bool,  CamSpringEdgeRotate).defaultValue(false).description("Rotate camera when cursor touches screen borders.");
CONFIG(float, CamSpringFastScaleMouseMove).defaultValue(3.0f / 10.0f).description("Scaling for CameraMoveFastMult in spring camera mode while moving mouse.");
CONFIG(float, CamSpringFastScaleMousewheelMove).defaultValue(2.0f / 10.0f).description("Scaling for CameraMoveFastMult in spring camera mode while scrolling with mouse.");
CONFIG(int,   CamSpringTrackMapHeightMode).defaultValue(HeightTracking::Terrain).description("Camera height is influenced by terrain height. 0=Static 1=Terrain 2=Smoothmesh");

static float DistanceToGround(float3 from, float3 dir, float fallbackPlaneHeight) {
	//ZoneScoped;
	float newGroundDist = CGround::LineGroundCol(from, from + dir * 150000.0f, false);

	// if the direction is not pointing towards the map we use provided xz plane as heuristic
	if (newGroundDist <= 0.0f)
		newGroundDist = CGround::LinePlaneCol(from, dir, 150000.0f, fallbackPlaneHeight);

	return newGroundDist;
}

CSpringController::CSpringController()
	: rot(2.677f, 0.0f, 0.0f)
	, curDist(float3(mapDims.mapx * 0.5f, 0.0f, mapDims.mapy * 0.55f).Length2D() * 1.5f * SQUARE_SIZE)
	, maxDist(std::max(mapDims.mapx, mapDims.mapy) * SQUARE_SIZE * 1.333f)
	, oldDist(0.0f)
	, zoomBack(false)
{
	//ZoneScoped;
	enabled = configHandler->GetBool("CamSpringEnabled");
	configHandler->NotifyOnChange(this, {"CamSpringScrollSpeed", "CamSpringFOV", "CamSpringZoomInToMousePos", "CamSpringZoomOutFromMousePos", "CamSpringFastScaleMousewheelMove", "CamSpringFastScaleMouseMove", "CamSpringEdgeRotate", "CamSpringLockCardinalDirections", "CamSpringTrackMapHeightMode"});
	ConfigUpdate();
}

CSpringController::~CSpringController()
{
	//ZoneScoped;
	configHandler->RemoveObserver(this);
}


void CSpringController::ConfigUpdate()
{
	//ZoneScoped;
	scrollSpeed = configHandler->GetFloat("CamSpringScrollSpeed") * 0.1f;
	fov = configHandler->GetFloat("CamSpringFOV");
	cursorZoomIn = configHandler->GetBool("CamSpringZoomInToMousePos");
	cursorZoomOut = configHandler->GetBool("CamSpringZoomOutFromMousePos");
	fastScaleMove = configHandler->GetFloat("CamSpringFastScaleMouseMove");
	fastScaleMousewheel = configHandler->GetFloat("CamSpringFastScaleMousewheelMove");
	doRotate = configHandler->GetBool("CamSpringEdgeRotate");
	lockCardinalDirections = configHandler->GetBool("CamSpringLockCardinalDirections");
	trackMapHeight = configHandler->GetInt("CamSpringTrackMapHeightMode");

	if (trackMapHeight == HeightTracking::Smooth && !modInfo.enableSmoothMesh) {
		LOG_L(L_ERROR, "Smooth mesh disabled");
		trackMapHeight = HeightTracking::Terrain;
	}
}

void CSpringController::ConfigNotify(const std::string & key, const std::string & value)
{
	//ZoneScoped;
	ConfigUpdate();
}

void CSpringController::SmoothCamHeight(const float3& prevPos) {
	//ZoneScoped;
	if (!pos.IsInBounds()) {
		return;
	}

	float3 camPos = GetPos(); // new camera pos with height from previous frame
	camPos.y = std::max(camPos.y, CGround::GetHeightReal(camPos.x, camPos.z, false) + 5.0f);

	// raycast to ground to simulate camera movement and find new point of focus
	const float distToGround = CGround::LineGroundCol(camPos, camPos + dir * 150000.0f, false);
	// FIXME camera focus is now first ground intersection which is not what we want
	// when there's a hill blocking the view
	const float3 newGroundPos = camPos + dir * distToGround;
	if (distToGround > 0.0f && newGroundPos.IsInBounds()) {
		const float camHeightDiff = (trackMapHeight == HeightTracking::Smooth) ?
			smoothGround.GetHeight(pos.x, pos.z) - smoothGround.GetHeight(prevPos.x, prevPos.z) :
			0.0f;

		pos = newGroundPos;
		curDist = distToGround + (dir * camHeightDiff).Length() * Sign(camHeightDiff);
	}
}

void CSpringController::KeyMove(float3 move)
{
	//ZoneScoped;
	move *= math::sqrt(move.z);

	const bool moveRotate = camHandler->GetActiveCamera()->GetMovState()[CCamera::MOVE_STATE_RTT];

	if (moveRotate) {
		rot.x = std::clamp(rot.x + move.y, math::PI * 0.51f, math::PI * 0.99f);
		MoveAzimuth(move.x);
		Update();
		return;
	}

	const float3 prevPos = pos;

	move *= 200.0f;
	const float3 flatForward = (dir * XZVector).ANormalize();
	pos += (camera->GetRight() * move.x + flatForward * move.y) * pixelSize * 2.0f * scrollSpeed;

	switch (trackMapHeight) {
	case HeightTracking::Terrain:
		// this is the default behavior as camera pos is based on:
		// - 'pos'      point of focus on the ground
		// - 'curDist'  camera distance
		break;
	case HeightTracking::Disabled:
		// freezing camera height requires raycasting from current
		// camera position and recalculating
		// point of focus and distance
		[[fallthrough]];
	case HeightTracking::Smooth:
		SmoothCamHeight(prevPos);
		break;
	}

	Update();
}


void CSpringController::MouseMove(float3 move)
{
	//ZoneScoped;
	// z is the speed modifier, in practice invertMouse{0,1} => move.z{-1,1}
	move.x *= move.z;
	move.y *= move.z;

	const bool moveFast = camHandler->GetActiveCamera()->GetMovState()[CCamera::MOVE_STATE_FST];

	move *= 0.005f;
	move *= (1 + moveFast * camera->moveFastMult * fastScaleMove);
	move.y = -move.y;
	move.z = 1.0f;

	KeyMove(move);
}


void CSpringController::ScreenEdgeMove(float3 move)
{
	//ZoneScoped;
	const bool belowMax = (mouse->lasty < globalRendering->viewSizeY /  3);
	const bool aboveMin = (mouse->lasty > globalRendering->viewSizeY / 10);
	const bool moveFast = camHandler->GetActiveCamera()->GetMovState()[CCamera::MOVE_STATE_FST];

	if (doRotate && aboveMin && belowMax) {
		// rotate camera when mouse touches top screen borders
		move *= (1 + moveFast * camera->moveFastMult * fastScaleMove);
		MoveAzimuth(move.x * 0.75f);
		move.x = 0.0f;
	}

	KeyMove(move);
}


void CSpringController::MouseWheelMove(float move, const float3& newDir)
{
	//ZoneScoped;
	const bool moveFast    = camHandler->GetActiveCamera()->GetMovState()[CCamera::MOVE_STATE_FST];
	const bool moveTilt    = camHandler->GetActiveCamera()->GetMovState()[CCamera::MOVE_STATE_TLT];
	const float shiftSpeed = (moveFast ? camera->moveFastMult * fastScaleMousewheel : 1.0f);
	const float scaledMove = 1.0f + (move * shiftSpeed * 0.007f);
	const float curDistPre = curDist;

	// tilt the camera if CTRL is pressed, otherwise zoom
	// no tweening during tilt, position is not fixed but
	// moves along an arc segment
	if (moveTilt) {
		rot.x -= (move * shiftSpeed * 0.005f);
	} else {
		// depends on curDist
		const float3 curCamPos = GetPos();

		curDist *= scaledMove;
		curDist = std::min(curDist, maxDist);

		float zoomTransTime = 0.25f;

		if (move < 0.0f) {
			// ZOOM IN - to mouse cursor or along our own forward dir
			zoomTransTime = ZoomIn(curCamPos, newDir, scaledMove);
		} else {
			// ZOOM OUT - from mid screen
			zoomTransTime = ZoomOut(curCamPos, newDir, curDistPre, scaledMove);
		}

		camHandler->CameraTransition(zoomTransTime);
	}

	Update();
}


float CSpringController::ZoomIn(const float3& curCamPos, const float3& newDir, const float& scaledMode)
{
	//ZoneScoped;
	const bool moveReset = camHandler->GetActiveCamera()->GetMovState()[CCamera::MOVE_STATE_RST];

	if (moveReset && zoomBack) {
		// instazoom in to standard view
		curDist = oldDist;
		zoomBack = false;

		return 0.5f;
	}

	if (!cursorZoomIn)
		return 0.25f;

	float curGroundDist = DistanceToGround(curCamPos, newDir, pos.y);
	if (curGroundDist <= 0.0f)
		return 0.25f;

	// zoom in to cursor, then back out (along same dir) based on scaledMove
	// to find where we want to place camera, but make sure the wanted point
	// is always in front of curCamPos
	const float3 cursorVec = newDir * curGroundDist;
	const float3 wantedPos = curCamPos + cursorVec * (1.0f - scaledMode);

	// figure out how far we will end up from the ground at new wanted point
	curDist = DistanceToGround(wantedPos, dir, pos.y);
	pos = wantedPos + dir * curDist;

	return 0.25f;
}

float CSpringController::ZoomOut(const float3& curCamPos, const float3& newDir, const float& curDistPre, const float& scaledMode)
{
	//ZoneScoped;
	const bool moveReset = camHandler->GetActiveCamera()->GetMovState()[CCamera::MOVE_STATE_RST];
	if (moveReset) {
		// instazoom out to maximum height
		if (!zoomBack) {
			oldDist = curDistPre;
			zoomBack = true;
		}

		rot = float3(2.677f, rot.y, 0.0f);
		pos.x = mapDims.mapx * SQUARE_SIZE * 0.5f;
		pos.z = mapDims.mapy * SQUARE_SIZE * 0.55f; // somewhat longer toward bottom
		curDist = pos.Length2D() * 1.5f;
		return 1.0f;
	}

	zoomBack = false;

	if (!cursorZoomOut)
		return 0.25f;

	const float zoomInDist = DistanceToGround(curCamPos, newDir, pos.y);

	if (zoomInDist <= 0.0f)
		return 0.25f;

	// same logic as ZoomIn, but in opposite direction
	const float3 cursorVec = newDir * zoomInDist;

	auto extrapolate_position = [&] (float scale) {
		const float3 wantedCamPos = curCamPos + cursorVec * (1.0f - scaledMode) * scale;
		const float newDist = DistanceToGround(wantedCamPos, dir, pos.y);
		return std::pair{wantedCamPos, newDist};
	};

	auto [wantedCamPos, newDist] = extrapolate_position(1.0);
	// don't move above the limit as camera height will be trimmed and the 
	// transition will not appear smooth
	if (newDist > maxDist) {
		// try to get as close as possible to the height limit
		std::tie(wantedCamPos, newDist) = extrapolate_position(0.5);
	}

	if (newDist > maxDist) {
		curDist = curDistPre;
		return 0.25f;
	}

	if (newDist > 0.0f)
		pos = wantedCamPos + dir * (curDist = newDist);

	return 0.25f;
}



void CSpringController::Update()
{
	//ZoneScoped;
	pos.ClampInMap();

	pos.y = CGround::GetHeightReal(pos.x, pos.z, false); // always focus on the ground
	rot.x = std::clamp(rot.x, math::PI * 0.51f, math::PI * 0.99f);

	// camera->SetRot(float3(rot.x, GetAzimuth(), rot.z));
	dir = CCamera::GetFwdFromRot(this->GetRot());

	curDist = std::clamp(curDist, 20.0f, maxDist);
	pixelSize = (camera->GetTanHalfFov() * 2.0f) / globalRendering->viewSizeY * curDist * 2.0f;
}


static float GetRotationWithCardinalLock(float rot)
{
	//ZoneScoped;
	constexpr float cardinalDirLockWidth = 0.2f;

	const float rotMoved = std::abs(rot /= math::HALFPI) - cardinalDirLockWidth * 0.5f;
	const float numerator = std::trunc(rotMoved);

	const float fract = rotMoved - numerator;
	const float b = 1.0f / (1.0f - cardinalDirLockWidth);
	const float c = 1.0f - b;
	const float fx = (fract > cardinalDirLockWidth) ? fract * b + c : 0.0f;

	return std::copysign(numerator + fx, rot) * math::HALFPI;
}


float CSpringController::MoveAzimuth(float move)
{
	//ZoneScoped;
	const float minRot  = std::floor(rot.y / math::HALFPI) * math::HALFPI;
	const float maxRot  = std::ceil(rot.y / math::HALFPI) * math::HALFPI;
	const bool moveTilt = camHandler->GetActiveCamera()->GetMovState()[CCamera::MOVE_STATE_TLT];

	rot.y -= move;

	if (lockCardinalDirections)
		return GetRotationWithCardinalLock(rot.y);
	if (moveTilt)
		rot.y = std::clamp(rot.y, minRot + 0.02f, maxRot - 0.02f);

	return rot.y;
}


float CSpringController::GetAzimuth() const
{
	//ZoneScoped;
	if (lockCardinalDirections)
		return GetRotationWithCardinalLock(rot.y);
	return rot.y;
}


float3 CSpringController::GetPos() const
{
	//ZoneScoped;
	const float3 cvec = dir * curDist;
	const float3 cpos = pos - cvec;
	return {cpos.x, std::max(cpos.y, CGround::GetHeightAboveWater(cpos.x, cpos.z, false) + 5.0f), cpos.z};
}


void CSpringController::SwitchTo(const int oldCam, const bool showText)
{
	//ZoneScoped;
	if (showText)
		LOG("Switching to Spring style camera");

	if (oldCam == CCameraHandler::CAMERA_MODE_OVERVIEW)
		return;

	rot = camera->GetRot() * XZVector;
}


void CSpringController::GetState(StateMap& sm) const
{
	//ZoneScoped;
	CCameraController::GetState(sm);
	sm["dist"] = curDist;
	sm["rx"]   = rot.x;
	sm["ry"]   = rot.y;
	sm["rz"]   = rot.z;
}


bool CSpringController::SetState(const StateMap& sm)
{
	//ZoneScoped;
	CCameraController::SetState(sm);
	SetStateFloat(sm, "dist", curDist);
	SetStateFloat(sm, "rx",   rot.x);
	SetStateFloat(sm, "ry",   rot.y);
	SetStateFloat(sm, "rz",   rot.z);
	return true;
}
