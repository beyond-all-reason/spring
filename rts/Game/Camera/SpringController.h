/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SPRING_CONTROLLER_H
#define _SPRING_CONTROLLER_H

#include "CameraController.h"
#include "Game/UI/MouseHandler.h"
#include "System/type2.h"

class CSpringController : public CCameraController
{
public:
	CSpringController();
	~CSpringController();

	const std::string GetName() const { return "spring"; }

	void KeyMove(float3 move);
	void MouseMove(float3 move);
	void ScreenEdgeMove(float3 move);
	void MouseWheelMove(float move) { MouseWheelMove(move, mouse->dir); }
	void MouseWheelMove(float move, const float3& newDir);

	void Update();
	void SetPos(const float3& newPos) { pos = newPos; Update(); }
	void SetRot(const float3& newRot) { rot = newRot; Update(); }
	float3 GetPos() const;
	float3 GetRot() const { return (float3(rot.x, GetAzimuth(), 0.0f)); }

	float3 SwitchFrom() const { return GetPos(); }
	void SwitchTo(const CCameraController* oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

private:
	float GetAzimuth() const;
	float MoveAzimuth(float move);

	inline float ZoomIn(const float3& curCamPos, const float3& dir, const float& curDistPre, const float& scaledMode);
	inline float ZoomOut(const float3& curCamPos, const float3& dir, const float& curDistPre, const float& scaledMode);

	void SmoothCamHeight(const float3& prevPos);

private:
	float3 rot;

	float curDist; // current zoom-out distance
	const float maxDist; // maximum zoom-out distance
	float minDist; // minimum zoom-in distance
	float oldDist;
	float fastScaleMove;
	float fastScaleMousewheel;

	bool zoomBack;
	bool cursorZoomIn;
	bool cursorZoomOut;
	bool doRotate;
	bool lockCardinalDirections;
	int trackMapHeight;
};

#endif // _SPRING_CONTROLLER_H
