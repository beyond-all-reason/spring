/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OVERHEAD_CONTROLLER_H
#define _OVERHEAD_CONTROLLER_H

#include "CameraController.h"
#include "Game/UI/MouseHandler.h"


class COverheadController : public CCameraController
{
public:
	COverheadController();
	~COverheadController();

	const std::string GetName() const { return "ta"; }

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move) { MouseWheelMove(move, mouse->dir); }
	void MouseWheelMove(float move, const float3& newDir);

	void Update();
	float3 GetPos() const { return (pos - dir * height); }
	void SetPos(const float3& newPos);
	void SetRot(const float3& newRot) { angle = std::clamp(math::PI - newRot.x, 0.01f, math::HALFPI); }

	float3 SwitchFrom() const { return GetPos(); }
	void SwitchTo(const CCameraController* oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

public:
	bool flipped;
private:
	bool changeAltHeight;

	float middleClickScrollSpeed;
	float height;
	float oldAltHeight;

	float maxHeight;
	float minHeight;
	float tiltSpeed;
	float angle;

	static constexpr float DEFAULT_ANGLE = 0.464f;

	float fastScale;
};

#endif // _OVERHEAD_CONTROLLER_H
