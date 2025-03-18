/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CAMERA_HANDLER_H
#define _CAMERA_HANDLER_H

#include <array>
#include <vector>
#include <string>

#include "Camera/CameraController.h"
#include "Game/Camera/DollyController.h"
#include "System/UnorderedMap.hpp"
#include "Console.h"

class CCamera;
class CPlayer;

class CCameraHandler : public CommandReceiver
{
public:
	typedef CCameraController::StateMap ViewData;

	enum {
		CAMERA_MODE_FIRSTPERSON = 0,
		CAMERA_MODE_OVERHEAD    = 1,
		CAMERA_MODE_SPRING      = 2,
		CAMERA_MODE_ROTOVERHEAD = 3,
		CAMERA_MODE_FREE        = 4,
		CAMERA_MODE_OVERVIEW    = 5,
		CAMERA_MODE_DOLLY       = 6,
		CAMERA_MODE_DUMMY       = 7,
		CAMERA_MODE_LAST        = 8,
	};
	enum {
		CAMERA_TRANSITION_MODE_EXP_DECAY = 0,
		CAMERA_TRANSITION_MODE_SPRING_DAMPENED = 1,
		CAMERA_TRANSITION_MODE_TIMED_SPRING_DAMPENED = 2,
		CAMERA_TRANSITION_MODE_LERP_SMOOTHED = 3,
	};

	struct CamTransitionState {
		float3 startPos;
		float3 tweenPos;
		float3 startRot;
		float3 tweenRot;

		float startFOV = 0.0f;
		float tweenFOV = 0.0f;

		float timeStart = 0.0f;
		float timeEnd = 0.0f;

		// spring dampened transitions
		float3 posVelocity;
		float3 rotVelocity;
		float fovVelocity;
		float lastTime;

		// configurable parameters
		float timeFactor = 0.0f;
		float timeExponent = 0.0f;
		float halflife = 0.0f;
	};


public:
	CCameraHandler();
	~CCameraHandler();


	static void InitStatic();
	static void KillStatic();
	static void SetActiveCamera(unsigned int camType);

	static CCamera* GetCamera(unsigned int camType);
	static CCamera* GetActiveCamera();

	// sets the current active camera, returns the previous
	static CCamera* GetSetActiveCamera(unsigned int camType) {
		CCamera* cam = GetActiveCamera(); SetActiveCamera(camType); return cam;
	}


	void Init();
	void Kill();
	void InitControllers();
	void KillControllers();
	void UpdateController(CPlayer* player, bool fpsMode);

	void SetCameraMode(unsigned int mode);
	void SetCameraMode(const std::string& mode);
	void PushMode();
	void PopMode();

	void ConfigNotify(const std::string& key, const std::string& value);

	void SetTransitionParams(float factor, float expon) {
		camTransState.timeFactor   = factor;
		camTransState.timeExponent =  expon;
  }
	void CameraTransition(float nsecs);
	void UpdateTransition();

	void ToggleState();
	void ToggleOverviewCamera();

	void PushAction(const Action&) override;

	void SaveView(const std::string& name);
	bool LoadView(const std::string& name);

	int GetModeIndex(const std::string& modeName) const;
	unsigned int GetCurrentControllerNum() const { return currCamCtrlNum; }

	float GetTransitionTimeFactor() const { return camTransState.timeFactor; }
	float GetTransitionTimeExponent() const { return camTransState.timeExponent; }


	/**
	 * @brief write current camera settings in a vector
	 */
	void GetState(CCameraController::StateMap& sm) const;
	CCameraController::StateMap GetState() const;

	/**
	 * @brief restore a camera state
	 * @param sm the state to set
	 * @return false when vector has wrong size or garbage data, true when applied without errors
	 */
	bool SetState(const CCameraController::StateMap& sm);


	const CCameraController& GetCurrentController() const { return *(camControllers[currCamCtrlNum]); }
	      CCameraController& GetCurrentController()       { return *(camControllers[currCamCtrlNum]); }

	CDollyController& GetDollyController() { return *(static_cast<CDollyController*>(camControllers[CAMERA_MODE_DOLLY])); }

	const std::array<CCameraController*, CAMERA_MODE_LAST>& GetControllers() const { return camControllers; }

private:
	void UpdateController(CCameraController& camCon, bool keyMove, bool wheelMove, bool edgeMove);
	bool LoadViewData(const ViewData& vd);

private:
	float3 lastCamRot;
	float3 lastCamPos;

	unsigned int currCamCtrlNum = CAMERA_MODE_DUMMY;
	unsigned int currCamTransitionNum = CAMERA_TRANSITION_MODE_EXP_DECAY;

	CamTransitionState camTransState;

	bool windowedEdgeMove = false;
	bool fullscreenEdgeMove = false;

	// last controller is a dummy
	std::array<CCameraController*, CAMERA_MODE_LAST> camControllers;
	std::vector<unsigned int> controllerStack;

	spring::unordered_map<std::string, ViewData> viewDataMap;
	spring::unordered_map<std::string, unsigned int> nameModeMap;
};

extern CCameraHandler* camHandler;

#endif // _CAMERA_HANDLER_H

