/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FPSController.h"

#include "Game/Camera.h"
#include "Game/GlobalUnsynced.h"
#include "Map/Ground.h"
#include "Map/ReadMap.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

#include <tracy/Tracy.hpp>

CONFIG(int, FPSScrollSpeed).defaultValue(10);
CONFIG(float, FPSMouseScale).defaultValue(0.01f);
CONFIG(bool, FPSEnabled).defaultValue(true);
CONFIG(float, FPSFOV).defaultValue(45.0f);
CONFIG(bool, FPSClampPos).defaultValue(true);


CFPSController::CFPSController(): oldHeight(300.0f)
{
	//ZoneScoped;
	ConfigUpdate();
	dir = camera->GetDir();

	configHandler->NotifyOnChange(this, {"FPSScrollSpeed", "FPSMouseScale", "FPSEnabled", "FPSFOV", "FPSClampPos"});
}

CFPSController::~CFPSController()
{
	//ZoneScoped;
	configHandler->RemoveObserver(this);
}

void CFPSController::ConfigUpdate()
{
	//ZoneScoped;
	scrollSpeed = configHandler->GetInt("FPSScrollSpeed") * 0.1f;
	mouseScale = configHandler->GetFloat("FPSMouseScale");
	enabled = configHandler->GetBool("FPSEnabled");
	fov = configHandler->GetFloat("FPSFOV");
	clampPos = configHandler->GetBool("FPSClampPos");
}

void CFPSController::ConfigNotify(const std::string& key, const std::string& value)
{
	//ZoneScoped;
	ConfigUpdate();
}


void CFPSController::KeyMove(float3 move)
{
	//ZoneScoped;
	move *= move.z * 400;
	pos  += (camera->GetDir() * move.y + camera->GetRight() * move.x) * scrollSpeed;
	Update();
}


void CFPSController::MouseMove(float3 move)
{
	//ZoneScoped;
	camera->SetRotY(camera->GetRot().y + mouseScale * move.x);
	camera->SetRotX(std::clamp(camera->GetRot().x + mouseScale * move.y * move.z, 0.01f, math::PI * 0.99f));
	dir = camera->GetDir();
	Update();
}


void CFPSController::MouseWheelMove(float move, const float3& newDir)
{
	//ZoneScoped;
	pos += (newDir * move);
	Update();
}


void CFPSController::Update()
{
	//ZoneScoped;
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
	//ZoneScoped;
	CCameraController::SetPos(newPos);

	if (!gu->fpsMode)
		pos.y = CGround::GetHeightAboveWater(pos.x, pos.z, false) + oldHeight;

	Update();
}


void CFPSController::SetDir(const float3& newDir)
{
	//ZoneScoped;
	dir = newDir;
	Update();
}


void CFPSController::SwitchTo(const int oldCam, const bool showText)
{
	//ZoneScoped;
	if (showText) {
		LOG("Switching to FPS style camera");
	}
}


void CFPSController::GetState(StateMap& sm) const
{
	//ZoneScoped;
	CCameraController::GetState(sm);
	sm["oldHeight"] = oldHeight;
}


bool CFPSController::SetState(const StateMap& sm)
{
	//ZoneScoped;
	CCameraController::SetState(sm);
	SetStateFloat(sm, "oldHeight", oldHeight);

	return true;
}


