/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keycode.h>
#include <tracy/Tracy.hpp>

#include "DollyController.h"
#include "Game/Camera.h"
#include "Game/Camera/CameraController.h"
#include "Game/CameraHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/UI/MouseHandler.h"
#include "Map/ReadMap.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Math/NURBS.h"
#include "System/MathConstants.h"
#include "System/Misc/SpringTime.h"
#include "System/Misc/TracyDefs.h"
#include "System/SpringMath.h"
#include "System/float3.h"

CONFIG(float, CamDollyFOV).defaultValue(45.0f);

CDollyController::CDollyController()
	: rot(2.677f, 0.0f, 0.0f)
	, lookTarget(0.f, 0.f, 0.f)
	, position(400.f, 400.f, 400.f)
{
	RECOIL_DETAILED_TRACY_ZONE;
	configHandler->NotifyOnChange(this, {"CamDollyFOV"});
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
	fov = configHandler->GetFloat("CamDollyFOV");
}

void CDollyController::ConfigNotify(const std::string& key, const std::string& value)
{
	RECOIL_DETAILED_TRACY_ZONE;
	ConfigUpdate();
}

void CDollyController::Run(float milliseconds)
{
	startTime = spring_gettime().toMilliSecsf();
	endTime = startTime + milliseconds;
	pauseTime = 0.f;
}

void CDollyController::Pause(float percent)
{
	if (percent >= 0.f && percent <= 1.f) {
		pauseTime = percent * (endTime - startTime) + startTime;
	} else {
		pauseTime = spring_gettime().toMilliSecsf();
	}
}

void CDollyController::Resume()
{
	float current = spring_gettime().toMilliSecsf();
	float duration = endTime - startTime;
	startTime = current - pauseTime + startTime;
	endTime = startTime + duration;
	pauseTime = 0.f;
}

void CDollyController::Update()
{
	ZoneScoped;
	float curTime = spring_gettime().toMilliSecsf();
	if (pauseTime > 0.f) {
		curTime = pauseTime;
	}
	float percent = 0;
	if (endTime != startTime) {
		percent = std::clamp(1 - (endTime - curTime) / (endTime - startTime), 0.f, 1.f);
	}
	if (mode == DOLLY_MODE_CURVE) {
		float minU = NURBS::minU(curveDegree, curveControlPoints, nurbsKnots);
		float maxU = NURBS::maxU(curveDegree, curveControlPoints, nurbsKnots);
		float u = minU + percent * (maxU - minU);
		pos = NURBS::SolveNURBS(curveDegree, curveControlPoints, nurbsKnots, u);
	} else if (mode == DOLLY_MODE_POSITION) {
		pos = position;
	}

	// LOG_L(L_INFO, "Dollypos: %s", pos.str().c_str());
	float relative = relmode - 1.0;
	if (lookMode == DOLLY_LOOKMODE_POSITION) {
		pos += lookTarget * relative;
		dir = (lookTarget - pos).Normalize();
	} else if (lookMode == DOLLY_LOOKMODE_CURVE) {
		float minU = NURBS::minU(lookCurveDegree, lookControlPoints, lookKnots);
		float maxU = NURBS::maxU(lookCurveDegree, lookControlPoints, lookKnots);
		float u = minU + percent * (maxU - minU);
		float3 lookT = NURBS::SolveNURBS(lookCurveDegree, lookControlPoints, lookKnots, u);
		pos += lookT * relative;
		dir = (lookT - pos).Normalize();
	} else if (lookMode == DOLLY_LOOKMODE_UNIT) {
		CUnit* unit = unitHandler.GetUnit(lookUnit);
		if (unit != nullptr && unit->IsInLosForAllyTeam(gu->myAllyTeam)) {
			pos += unit->drawPos * relative;
			dir = (unit->drawPos - pos).Normalize();
		}
	}

	float3 newRot = CCamera::GetRotFromDir(GetDir());
	int wraps = std::trunc(rot.y / math::TWOPI);
	newRot.y += math::TWOPI * wraps;
	float ydiff = rot.y - newRot.y;
	if (abs(ydiff) > math::PI) {
		newRot.y += math::TWOPI * Sign(ydiff);
	}
	rot = newRot;
	/* Note, even an epsilon value here will make the camera
	 * fail to track when smoothness is set high enough */
	camHandler->CameraTransition(0.0f);
}

void CDollyController::SwitchTo(const CCameraController* oldCam, const bool showText)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (showText)
		LOG("Switching to Dolly style camera");
	startTime = endTime = 0.f;
	rot = oldCam->GetRot();
}

void CDollyController::SetNURBS(int degree, const std::vector<float4>& cpoints,
                                const std::vector<float>& knots)
{
	curveDegree = degree;
	curveControlPoints = cpoints;
	nurbsKnots = knots;
}

void CDollyController::SetLookPosition(const float3& pos)
{
	lookTarget = pos;
}

void CDollyController::SetLookUnit(int unitid)
{
	lookUnit = unitid;
}

void CDollyController::SetLookCurve(int degree, const std::vector<float4>& cpoints,
                                    const std::vector<float>& knots)
{
	lookCurveDegree = degree;
	lookControlPoints = cpoints;
	lookKnots = knots;
}

void CDollyController::GetState(StateMap& sm) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::GetState(sm);
	sm["rx"]   = rot.x;
	sm["ry"]   = rot.y;
	sm["rz"]   = rot.z;
}

bool CDollyController::SetState(const StateMap& sm)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::SetState(sm);
	SetStateFloat(sm, "rx", rot.x);
	SetStateFloat(sm, "ry", rot.y);
	SetStateFloat(sm, "rz", rot.z);
	return true;
}
