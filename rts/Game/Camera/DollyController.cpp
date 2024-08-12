/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL_keycode.h>
#include <tracy/Tracy.hpp>

#include "DollyController.h"
#include "Game/Camera.h"
#include "Game/Camera/CameraController.h"
#include "Game/CameraHandler.h"
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
	, lookTarget(float3(500.f, 100.f, 500.f))
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

void CDollyController::Update()
{
	ZoneScoped;
	float curTime = spring_gettime().toMilliSecsf();
	float percent = std::clamp(1 - (endTime - curTime) / (endTime - startTime), 0.f, 1.f);
	if (mode == DOLLY_MODE_CURVE) {
		float minU = NURBS::minU(curveDegree, curveControlPoints, nurbsKnots);
		float maxU = NURBS::maxU(curveDegree, curveControlPoints, nurbsKnots);
		float u = minU + percent * (maxU - minU);
		pos = NURBS::SolveNURBS(curveDegree, curveControlPoints, nurbsKnots, u);
	}
	// LOG_L(L_INFO, "Dollypos: %s", pos.str().c_str());

	if (lookMode == DOLLY_LOOKMODE_POSITION) {
		dir = (lookTarget - pos).Normalize();
	} else if (lookMode == DOLLY_LOOKMODE_CURVE) {
		float minU = NURBS::minU(lookCurveDegree, lookControlPoints, lookKnots);
		float maxU = NURBS::maxU(lookCurveDegree, lookControlPoints, lookKnots);
		float u = minU + percent * (maxU - minU);
		float3 lookT = NURBS::SolveNURBS(lookCurveDegree, lookControlPoints, lookKnots, u);
		dir = (lookT - pos).Normalize();
	} else if (lookMode == DOLLY_LOOKMODE_UNIT) {
		CUnit* unit = unitHandler.GetUnit(lookUnit);
		if (unit != nullptr) {
			dir = (unit->midPos - pos).Normalize();
		}
	}
	float3 newRot = CCamera::GetRotFromDir(GetDir());
	float3 toward = GetRadAngleToward(rot, newRot);
	if (std::abs(toward.x) > math::PI) {
		toward.x += math::TWOPI * Sign(toward.x);
	}
	if (std::abs(toward.y) > math::PI) {
		toward.y += math::TWOPI * Sign(toward.x);
	}
	if (std::abs(toward.z) > math::PI) {
		toward.z += math::TWOPI * Sign(toward.x);
	}
	// LOG_L(L_INFO, "Dollyrot: %s", newRot.str().c_str());
	rot = rot + toward;
	camHandler->CameraTransition(0.01f);
}

void CDollyController::SwitchTo(const CCameraController* oldCam, const bool showText)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (showText)
		LOG("Switching to Dolly style camera");

	startTime = spring_gettime().toMilliSecsf();
	endTime = startTime + 10000;
}

void CDollyController::SetNURBS(int degree, std::vector<float4> cpoints, std::vector<float> knots)
{
	curveDegree = degree;
	curveControlPoints = cpoints;
	nurbsKnots = knots;
}

void CDollyController::SetLookMode(int mode)
{
	if (mode < DOLLY_LOOKMODE_POSITION || mode > DOLLY_LOOKMODE_CURVE) {
		mode = 1;
	}
	lookMode = mode;
}

void CDollyController::SetLookPosition(float3 pos)
{
	lookTarget = pos;
}

void CDollyController::SetLookUnit(int unitid)
{
	lookUnit = unitid;
}

void CDollyController::SetLookCurve(int degree, std::vector<float4> cpoints,
                                    std::vector<float> knots)
{
	lookCurveDegree = degree;
	lookControlPoints = cpoints;
	lookKnots = knots;
}

void CDollyController::GetState(StateMap& sm) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::GetState(sm);
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
