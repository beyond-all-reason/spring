/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OV_CONTROLLER_H
#define _OV_CONTROLLER_H

#include "CameraController.h"

class COverviewController : public CCameraController
{
public:
	COverviewController();
	~COverviewController();

	constexpr static float3 DIR_UP = float3(0.0f, -0.999999500000375f, -0.000999999500000375f); // -90 deg
	constexpr static float3 DIR_LEFT = float3(0.000999999500000375f, -0.999999500000375f, 0.0f); // -90 deg
	constexpr static float3 DIR_BOTTOM = float3(0.0f, -0.999999500000375f, 0.000999999500000375f); // 90 deg
	constexpr static float3 DIR_RIGHT = float3(-0.000999999500000375f, -0.999999500000375f, 0.0f); // -90 deg

	const std::string GetName() const { return "ov"; }

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

	void KeyMove(float3 move) override {}
	void MouseMove(float3 move) override {}
	void ScreenEdgeMove(float3 move) override {}
	void MouseWheelMove(float move) override {}
	void MouseWheelMove(float move, const float3& newDir) override { }

	void SetPos(const float3& newPos) override {}
	void SetDir(const float3& newDir) override {}

	float3 SwitchFrom() const;
	void SwitchTo(const int oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

private:
	bool minimizeMinimap;
	bool followRotation;
};

#endif // _OV_CONTROLLER_H
