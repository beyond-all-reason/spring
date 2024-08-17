/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DOLLY_CONTROLLER_H
#define _DOLLY_CONTROLLER_H

#include "CameraController.h"
#include "Game/UI/MouseHandler.h"

class CDollyController : public CCameraController
{
public:
	CDollyController();
	~CDollyController();
	enum {
		DOLLY_MODE_POSITION = 1,
		DOLLY_MODE_CURVE = 2,
	};
	enum {
		DOLLY_RELATIVE_WORLD = 1,
		DOLLY_RELATIVE_TARGET = 2,
	};
	enum {
		DOLLY_LOOKMODE_POSITION = 1,
		DOLLY_LOOKMODE_UNIT = 2,
		DOLLY_LOOKMODE_CURVE = 3,
	};
	const std::string GetName() const
	{
		return "dolly";
	}

	void KeyMove(float3 move) {};
	void MouseMove(float3 move) {};
	void ScreenEdgeMove(float3 move) {};
	void MouseWheelMove(float move) {};
	void MouseWheelMove(float move, const float3& newDir) {};

	void Update();
	void SetPos(const float3& newPos)
	{
		pos = newPos;
	};
	void SetRot(const float3& newRot)
	{
		rot = newRot;
	};
	float3 GetRot() const
	{
		return rot;
	};

	float3 SwitchFrom() const
	{
		return pos;
	};
	void SwitchTo(const CCameraController* oldCam, const bool showText);

	void Run(float milliseconds);
	void Pause(float percent);
	void Resume();
	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

	void SetMode(int newMode) { mode = std::clamp(newMode, 1, 2); }
	void SetRelativeMode(int newMode) { relmode = std::clamp(newMode, 1, 2); }
	void SetPosition(const float3& newPosition) { position = newPosition; };
	void SetNURBS(int degree, const std::vector<float4> &cpoints, const std::vector<float> &knots);

	void SetLookMode(int newMode) { lookMode = std::clamp(newMode, 1, 3); };
	void SetLookPosition(const float3 &pos);
	void SetLookUnit(int unitid);
	void SetLookCurve(int degree, const std::vector<float4> &cpoints, const std::vector<float> &knots);

private:
	float3 rot;
	int mode = 1;
	int relmode = 1;
	float3 position;

	std::vector<float4> curveControlPoints;
	std::vector<float> nurbsKnots;

	int lookMode = 1;
	float3 lookTarget;
	int lookUnit;
	int lookCurveDegree;
	std::vector<float4> lookControlPoints;
	std::vector<float> lookKnots;

	int curveDegree = 3;
	float startTime = 1.;
	float endTime = 1.;
	float pauseTime = 0.;
};

#endif  // _DOLLY_CONTROLLER_H
