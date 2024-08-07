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
	void MouseWheelMove(float move)
	{
	}
	void MouseWheelMove(float move, const float3& newDir) {};

	void Update();
	void SetPos(const float3& newPos)
	{
		pos = newPos;
	}
	void SetRot(const float3& newRot)
	{
		rot = newRot;
	}
	// float3 GetPos() const;
	float3 GetRot()
	{
		return rot;
	};

	float3 SwitchFrom() const
	{
		return pos;
	}
	void SwitchTo(const int oldCam, const bool showText);

	void GetState(StateMap& sm) const;
	bool SetState(const StateMap& sm);

	void ConfigNotify(const std::string& key, const std::string& value);
	void ConfigUpdate();

	void SetNURBS(int degree, std::vector<float4> cpoints, std::vector<float> knots);

	void SetLookMode(int mode);
	void SetLookPosition(float3 pos);
	void SetLookUnit(int unitid);
	void SetLookCurve(int degree, std::vector<float4> cpoints, std::vector<float> knots);

private:
	float3 rot;

	float curDist;        // current zoom-out distance
	const float maxDist;  // maximum zoom-out distance
	float oldDist;
	float fastScaleMove;
	float fastScaleMousewheel;

	bool zoomBack;
	bool cursorZoomIn;
	bool cursorZoomOut;
	bool doRotate;
	bool lockCardinalDirections;
	int trackMapHeight;

	std::vector<float4> curveControlPoints = {{-4.0, -4.0, 0.0, 1.0},
	                                          {-2.0, 4.0, 0.0, 1.0},
	                                          {2.0, -4.0, 0.0, 1.0},
	                                          {4.0, 4.0, 0.0, 1.0}};
	std::vector<float> nurbsKnots = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};

	int lookMode = 1;
	float3 lookTarget;
	int lookUnit;
	int lookCurveDegree;
	std::vector<float4> lookControlPoints = {{-4.0, -4.0, 0.0, 1.0},
	                                         {-2.0, 4.0, 0.0, 1.0},
	                                         {2.0, -4.0, 0.0, 1.0},
	                                         {4.0, 4.0, 0.0, 1.0}};
	std::vector<float> lookKnots = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};

	int curveDegree = 3;
	float startTime = 1.;
	float endTime = 1.;
};

#endif  // _DOLLY_CONTROLLER_H
