/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OV_CONTROLLER_H
#define _OV_CONTROLLER_H

#include "CameraController.h"

class COverviewController : public CCameraController
{
public:
	COverviewController();
	~COverviewController();

	const std::string GetName() const { return "ov"; }

	void KeyMove(float3 move) override {}
	void MouseMove(float3 move) override {}
	void ScreenEdgeMove(float3 move) override {}
	void MouseWheelMove(float move) override {}
	void MouseWheelMove(float move, const float3& newDir) override { }

	float3 GetRot() const override;

	void SetPos(const float3& newPos) override {}
	void SetDir(const float3& newDir) override {}
	void SetRot(const float3& newDir) override {camRotY = newDir.y;}

	float3 SwitchFrom() const;
	void SwitchTo(const CCameraController* oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

private:
	bool minimizeMinimap;
	bool dynamicRotation;
	float camRotY;
};

#endif // _OV_CONTROLLER_H
