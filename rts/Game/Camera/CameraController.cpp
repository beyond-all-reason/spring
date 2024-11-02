/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "CameraController.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/Config/ConfigHandler.h"

#include "System/Misc/TracyDefs.h"


CONFIG(float, UseDistToGroundForIcons).defaultValue(0.95f);


CCameraController::CCameraController()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// switchVal:
	// * 1.0 = 0 degree  = overview
	// * 0.0 = 90 degree = first person
	switchVal = configHandler->GetFloat("UseDistToGroundForIcons");
	scrollSpeed = 1.0f;
	fov = 45.0f;
	pixelSize = 1.0f;

	enabled = true;

	pos = float3(mapDims.mapx * 0.5f * SQUARE_SIZE, 1000.0f, mapDims.mapy * 0.5f * SQUARE_SIZE); // center map
	dir = FwdVector;
}

float3 CCameraController::GetRot() const { return CCamera::GetRotFromDir(GetDir()); }
void CCameraController::SetRot(const float3& newRot) { dir = CCamera::GetFwdFromRot(newRot); }


bool CCameraController::SetStateBool(const StateMap& sm, const std::string& name, bool& var)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const StateMap::const_iterator it = sm.find(name);

	if (it != sm.cend()) {
		var = (it->second > 0.0f);
		return true;
	}

	return false;
}

bool CCameraController::SetStateFloat(const StateMap& sm, const std::string& name, float& var)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const StateMap::const_iterator it = sm.find(name);

	if (it != sm.cend()) {
		var = it->second;
		return true;
	}

	return false;
}


// Uses distance to ground for large angles (near 90 degree),
// and distance to unit for flat angles (near 0 degree),
// when comparing the camera direction to the map surface,
// assuming the map is flat.
bool CCameraController::GetUseDistToGroundForIcons() {
	// dir should already be normalized
	const float rawDot = UpVector.dot(GetDir());
	const float absDot = std::clamp(math::fabs(rawDot), 0.0f, 1.0f);

	// dot< switch: flat angle (typical for first person camera)
	// dot>=switch: steep angle (typical for overhead camera)
	return (absDot >= switchVal);
}



bool CCameraController::SetState(const StateMap& sm)
{
	RECOIL_DETAILED_TRACY_ZONE;
	SetStateFloat(sm, "fov", fov);

	SetStateFloat(sm, "px", pos.x);
	SetStateFloat(sm, "py", pos.y);
	SetStateFloat(sm, "pz", pos.z);

	SetStateFloat(sm, "dx", dir.x);
	SetStateFloat(sm, "dy", dir.y);
	SetStateFloat(sm, "dz", dir.z);

	return true;
}

void CCameraController::GetState(StateMap& sm) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	sm["fov"] = fov;

	sm["px"] = pos.x;
	sm["py"] = pos.y;
	sm["pz"] = pos.z;

	sm["dx"] = dir.x;
	sm["dy"] = dir.y;
	sm["dz"] = dir.z;
}

float CCameraController::DistanceToGround(float3 from, float3 dir, float fallbackPlaneHeight) {
	RECOIL_DETAILED_TRACY_ZONE;
	float newGroundDist = CGround::LineGroundCol(from, from + dir * 150000.0f, false);

	// if the direction is not pointing towards the map we use provided xz plane as heuristic
	if (newGroundDist <= 0.0f) {
		newGroundDist = CGround::LinePlaneCol(from, dir, 150000.0f, fallbackPlaneHeight);
	}

	return newGroundDist;
}