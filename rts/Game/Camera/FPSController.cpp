/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FPSController.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

#include "System/Misc/TracyDefs.h"

CONFIG(int, FPSScrollSpeed).defaultValue(10);
CONFIG(float, FPSMouseScale).defaultValue(0.01f);
CONFIG(bool, FPSEnabled).defaultValue(true);
CONFIG(float, FPSFOV).defaultValue(45.0f);
CONFIG(bool, FPSClampPos).defaultValue(true);


CFPSController::CFPSController(): oldHeight(300.0f), rot(2.677f, 0.0f, 0.0f)
{
	RECOIL_DETAILED_TRACY_ZONE;
	ConfigUpdate();
	dir = camera->GetDir();

	configHandler->NotifyOnChange(this, {"FPSScrollSpeed", "FPSMouseScale", "FPSEnabled", "FPSFOV", "FPSClampPos"});
}

CFPSController::~CFPSController()
{
	RECOIL_DETAILED_TRACY_ZONE;
	configHandler->RemoveObserver(this);
}

void CFPSController::ConfigUpdate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	scrollSpeed = configHandler->GetInt("FPSScrollSpeed") * 0.1f;
	mouseScale = configHandler->GetFloat("FPSMouseScale");
	enabled = configHandler->GetBool("FPSEnabled");
	fov = configHandler->GetFloat("FPSFOV");
	clampPos = configHandler->GetBool("FPSClampPos");
}

void CFPSController::ConfigNotify(const std::string& key, const std::string& value)
{
	RECOIL_DETAILED_TRACY_ZONE;
	ConfigUpdate();
}


void CFPSController::KeyMove(float3 move)
{
	RECOIL_DETAILED_TRACY_ZONE;
	move *= move.z * 400;
	pos  += (camera->GetDir() * move.y + camera->GetRight() * move.x) * scrollSpeed;
	Update();
}


void CFPSController::MouseMove(float3 move)
{
	RECOIL_DETAILED_TRACY_ZONE;
	rot.y += mouseScale * move.x;
	rot.x = std::clamp(rot.x + mouseScale * move.y * move.z, 0.01f, math::PI * 0.99f);
	Update();
}


void CFPSController::MouseWheelMove(float move, const float3& newDir)
{
	RECOIL_DETAILED_TRACY_ZONE;
	pos += (newDir * move);
	Update();
}


void CFPSController::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (gu->fpsMode || !clampPos)
		return;

	const float margin = 0.01f;
	const float xMin = margin;
	const float zMin = margin;
	const float xMax = (float)(mapDims.mapx * SQUARE_SIZE) - margin;
	const float zMax = (float)(mapDims.mapy * SQUARE_SIZE) - margin;

	pos.x = std::clamp(pos.x, xMin, xMax);
	pos.z = std::clamp(pos.z, zMin, zMax);

	const float gndHeight = CGround::GetHeightAboveWater(pos.x, pos.z, false);
	const float yMin = gndHeight + 5.0f;
	const float yMax = 9000.0f;

	pos.y = std::clamp(pos.y, yMin, yMax);
	oldHeight = pos.y - gndHeight;
}


void CFPSController::SetPos(const float3& newPos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::SetPos(newPos);

	if (!gu->fpsMode)
		pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + oldHeight;

	Update();
}


void CFPSController::SetDir(const float3& newDir)
{
	RECOIL_DETAILED_TRACY_ZONE;
	dir = newDir;
	rot = CCamera::GetRotFromDir(newDir);
}

void CFPSController::SetRot(const float3& newRot)
{
	RECOIL_DETAILED_TRACY_ZONE;
	rot = newRot;
	dir = CCamera::GetFwdFromRot(newRot);
}


void CFPSController::SwitchTo(const CCameraController* oldCam, const bool showText)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (showText) {
		LOG("Switching to FPS style camera");
	}
	float3 newPos = oldCam->SwitchFrom();
	if (oldCam->GetName() == "ov") {
		pos = float3(newPos.x, pos.y, newPos.z);
		Update();
		return;
	}
	rot = oldCam->GetRot();
	pos = newPos;
}


void CFPSController::GetState(StateMap& sm) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::GetState(sm);
	sm["oldHeight"] = oldHeight;
	sm["rx"]   = rot.x;
	sm["ry"]   = rot.y;
	sm["rz"]   = rot.z;
}


bool CFPSController::SetState(const StateMap& sm)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::SetState(sm);
	SetStateFloat(sm, "oldHeight", oldHeight);
	SetStateFloat(sm, "rx",   rot.x);
	SetStateFloat(sm, "ry",   rot.y);
	SetStateFloat(sm, "rz",   rot.z);

	return true;
}


