/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "RotOverheadController.h"

#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/SpringMath.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#include "Game/CameraHandler.h"

#include "System/Misc/TracyDefs.h"

CONFIG(float, RotOverheadMouseScale).defaultValue(0.01f);
CONFIG(int, RotOverheadScrollSpeed).defaultValue(10);
CONFIG(bool, RotOverheadEnabled).defaultValue(true).headlessValue(false);
CONFIG(float, RotOverheadFOV).defaultValue(45.0f);
CONFIG(bool, RotOverheadClampMap).defaultValue(true).headlessValue(true);


CRotOverheadController::CRotOverheadController(): oldHeight(500.0f), rot(2.677f, 0.0f, 0.0f)
{
	RECOIL_DETAILED_TRACY_ZONE;
	mouseScale  = configHandler->GetFloat("RotOverheadMouseScale");
	scrollSpeed = configHandler->GetInt("RotOverheadScrollSpeed") * 0.1f;
	enabled     = configHandler->GetBool("RotOverheadEnabled");
	fov         = configHandler->GetFloat("RotOverheadFOV");
	clampToMap = configHandler->GetBool("RotOverheadClampMap");
}


void CRotOverheadController::KeyMove(float3 move)
{
	RECOIL_DETAILED_TRACY_ZONE;
	move *= math::sqrt(move.z) * 400;

	float3 flatForward = camera->GetDir();
	if(camera->GetDir().y < -0.9f)
		flatForward += camera->GetUp();
	flatForward.y = 0;
	flatForward.ANormalize();

	pos += (flatForward * move.y + camera->GetRight() * move.x) * scrollSpeed;
	Update();
}


void CRotOverheadController::MouseMove(float3 move)
{
	RECOIL_DETAILED_TRACY_ZONE;
	rot.x = std::clamp(rot.x + mouseScale * move.y * move.z, math::PI * 0.4999f, math::PI * 0.9999f);
	rot.y += mouseScale * move.x ;
	dir = CCamera::GetFwdFromRot(rot);
	Update();
}


void CRotOverheadController::ScreenEdgeMove(float3 move)
{
	RECOIL_DETAILED_TRACY_ZONE;
	KeyMove(move);
}


void CRotOverheadController::MouseWheelMove(float move)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const float gheight = CGround::GetHeightAboveWater(pos.x, pos.z, false);
	float height = pos.y - gheight;

	height *= (1.0f + (move * mouseScale));
	pos.y = height + gheight;

	Update();
}

void CRotOverheadController::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (clampToMap) {
		pos.x = std::clamp(pos.x, 0.01f, mapDims.mapx * SQUARE_SIZE - 0.01f);
		pos.z = std::clamp(pos.z, 0.01f, mapDims.mapy * SQUARE_SIZE - 0.01f);
	}
	else {
		pos.x = std::clamp(pos.x, -1.0f * mapDims.mapx * SQUARE_SIZE, 2.0f * mapDims.mapx * SQUARE_SIZE - 0.01f);
		pos.z = std::clamp(pos.z, -1.0f * mapDims.mapy * SQUARE_SIZE, 2.0f * mapDims.mapy * SQUARE_SIZE - 0.01f);
	}

	float h = CGround::GetHeightAboveWater(pos.x, pos.z, false);
	pos.y = std::clamp(pos.y, h + 5, 9000.0f);
	oldHeight = pos.y - h;
}

void CRotOverheadController::SetPos(const float3& newPos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::SetPos(newPos);
	pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + oldHeight;
	Update();
}


float3 CRotOverheadController::SwitchFrom() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	return pos;
}


void CRotOverheadController::SwitchTo(const CCameraController* oldCam, const bool showText)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (showText) {
		LOG("Switching to Rotatable overhead camera");
	}
	float3 newPos = oldCam->SwitchFrom();
	if (oldCam->GetName() == "ov") {
		pos = float3(newPos.x, pos.y, newPos.z);
		Update();
		return;
	}
	pos = newPos;
	rot = oldCam->GetRot();
}


void CRotOverheadController::GetState(StateMap& sm) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::GetState(sm);

	sm["oldHeight"] = oldHeight;
	sm["rx"]   = rot.x;
	sm["ry"]   = rot.y;
	sm["rz"]   = rot.z;
}


bool CRotOverheadController::SetState(const StateMap& sm)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::SetState(sm);

	SetStateFloat(sm, "oldHeight", oldHeight);
	SetStateFloat(sm, "rx",   rot.x);
	SetStateFloat(sm, "ry",   rot.y);
	SetStateFloat(sm, "rz",   rot.z);

	return true;
}

