/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FPS_CONTROLLER_H
#define _FPS_CONTROLLER_H

#include "Game/Camera.h"
#include "CameraController.h"

class CFPSController : public CCameraController
{
public:
	CFPSController();
	~CFPSController();

	const std::string GetName() const { return "fps"; }

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move) { KeyMove(move); }
	void MouseWheelMove(float move) { MouseWheelMove(move, camera->GetUp()); }
	void MouseWheelMove(float move, const float3& newDir);

	void SetPos(const float3& newPos);
	void SetDir(const float3& newDir);
	void SetRot(const float3& newRot);
	float3 GetRot() const { return rot; }
	float3 SwitchFrom() const { return pos; }
	void SwitchTo(const CCameraController* oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);


	void Update();

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

private:
	float3 rot;
	float mouseScale;
	float oldHeight;
	bool clampPos;
};

#endif // _FPS_CONTROLLER_H
