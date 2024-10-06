/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cmath>
#include <cstring> // memset
#include <cstdlib>
#include <cstdarg> // va_start

#include "CameraHandler.h"

#include "Action.h"
#include "Camera.h"
#include "Camera/CameraController.h"
#include "Camera/DummyController.h"
#include "Camera/FPSController.h"
#include "Camera/OverheadController.h"
#include "Camera/RotOverheadController.h"
#include "Camera/FreeController.h"
#include "Camera/OverviewController.h"
#include "Camera/SpringController.h"
#include "Players/Player.h"
#include "System/MathConstants.h"
#include "UI/UnitTracker.h"
#include "Rendering/GlobalRendering.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringHash.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Math/SpringDampers.h"
#include "System/Math/expDecay.h"

#include "System/Misc/TracyDefs.h"


static std::string strformat(const char* fmt, ...)
{
	char buf[256];
	va_list args;
	va_start(args,fmt);
	VSNPRINTF(buf, sizeof(buf), fmt, args);
	va_end(args);
	return std::string(buf);
}

CONFIG(std::string, CamModeName).defaultValue("");

CONFIG(int, CamMode)
	.defaultValue(CCameraHandler::CAMERA_MODE_SPRING)
	.description(strformat("Defines the used camera. Options are:\n%i = FPS\n%i = Overhead\n%i = Spring\n%i = RotOverhead\n%i = Free\n%i = Overview",
		(int)CCameraHandler::CAMERA_MODE_FIRSTPERSON,
		(int)CCameraHandler::CAMERA_MODE_OVERHEAD,
		(int)CCameraHandler::CAMERA_MODE_SPRING,
		(int)CCameraHandler::CAMERA_MODE_ROTOVERHEAD,
		(int)CCameraHandler::CAMERA_MODE_FREE,
		(int)CCameraHandler::CAMERA_MODE_OVERVIEW
	))
	.minimumValue(0)
	.maximumValue(CCameraHandler::CAMERA_MODE_DUMMY - 1);

CONFIG(bool, WindowedEdgeMove).defaultValue(true).description("Sets whether moving the mouse cursor to the screen edge will move the camera across the map.");
CONFIG(bool, FullscreenEdgeMove).defaultValue(true).description("see WindowedEdgeMove, just for fullscreen mode");

CONFIG(float, CamTimeFactor)
	.defaultValue(1.0f)
	.minimumValue(0.0f)
	.description("Scales the speed of camera transitions, e.g. zooming or position change.");

CONFIG(float, CamTimeExponent)
	.defaultValue(4.0f)
	.minimumValue(0.0f)
	.description("Camera transitions happen at lerp(old, new, timeNorm ^ CamTimeExponent).");

CONFIG(int, CamTransitionMode)
	.defaultValue(CCameraHandler::CAMERA_TRANSITION_MODE_EXP_DECAY)
	.description(strformat("Defines the function used for camera transitions. Options are:\n%i = Exponential Decay\n%i = Spring Dampened\n%i = Spring Dampened with timed transitions\n%i = Lerp Smoothed",
		(int)CCameraHandler::CAMERA_TRANSITION_MODE_EXP_DECAY,
		(int)CCameraHandler::CAMERA_TRANSITION_MODE_SPRING_DAMPENED,
		(int)CCameraHandler::CAMERA_TRANSITION_MODE_TIMED_SPRING_DAMPENED,
		(int)CCameraHandler::CAMERA_TRANSITION_MODE_LERP_SMOOTHED))
	.minimumValue(0)
	.maximumValue((int)CCameraHandler::CAMERA_TRANSITION_MODE_LERP_SMOOTHED);

CONFIG(int, CamSpringHalflife)
	.defaultValue(100)
	.description("For Spring Dampened camera. It is the time in milliseconds at which the camera should be approximately halfway towards the goal.")
	.minimumValue(0);

CCameraHandler* camHandler = nullptr;




// cameras[ACTIVE] is just used to store which of the others is active
static CCamera cameras[CCamera::CAMTYPE_COUNT];

alignas (CFreeController) static std::byte camControllerMem[CCameraHandler::CAMERA_MODE_LAST][sizeof(CFreeController)];
alignas (CCameraHandler) static std::byte camHandlerMem[sizeof(CCameraHandler)];


void CCameraHandler::SetActiveCamera(unsigned int camType) { cameras[CCamera::CAMTYPE_ACTIVE].SetCamType(camType); }
void CCameraHandler::InitStatic() {
	// initialize all global cameras
	for (unsigned int i = CCamera::CAMTYPE_PLAYER; i < CCamera::CAMTYPE_COUNT; i++) {
		cameras[i].SetCamType(i);
		cameras[i].SetProjType((i == CCamera::CAMTYPE_SHADOW)? CCamera::PROJTYPE_ORTHO: CCamera::PROJTYPE_PERSP);
		cameras[i].SetClipCtrlMatrix(CMatrix44f::ClipControl(globalRendering->supportClipSpaceControl));
	    cameras[i].InitConfigNotify();
	}

	SetActiveCamera(CCamera::CAMTYPE_PLAYER);

	camHandler = new (camHandlerMem) CCameraHandler();
}

void CCameraHandler::KillStatic() {
	RECOIL_DETAILED_TRACY_ZONE;
	for (unsigned int i = CCamera::CAMTYPE_PLAYER; i < CCamera::CAMTYPE_COUNT; i++) {
	    cameras[i].RemoveConfigNotify();
	}
	spring::SafeDestruct(camHandler);
	std::memset(camHandlerMem, 0, sizeof(camHandlerMem));
}


CCamera* CCameraHandler::GetCamera(unsigned int camType) { return &cameras[camType]; }
CCamera* CCameraHandler::GetActiveCamera() { return (GetCamera(cameras[CCamera::CAMTYPE_ACTIVE].GetCamType())); }



// some controllers access the heightmap, do not construct them yet
CCameraHandler::CCameraHandler() {
	RECOIL_DETAILED_TRACY_ZONE;
	camControllers.fill(nullptr);
	camControllers[CAMERA_MODE_DUMMY] = new (camControllerMem[CAMERA_MODE_DUMMY]) CDummyController();
}

CCameraHandler::~CCameraHandler() {
	RECOIL_DETAILED_TRACY_ZONE;
	// regular controllers should already have been killed
	assert(camControllers[0] == nullptr);
	spring::SafeDestruct(camControllers[CAMERA_MODE_DUMMY]);
	std::memset(camControllerMem[CAMERA_MODE_DUMMY], 0, sizeof(camControllerMem[CAMERA_MODE_DUMMY]));
}


void CCameraHandler::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	{
		InitControllers();

		for (unsigned int i = 0; i < CAMERA_MODE_DUMMY; i++) {
			nameModeMap[camControllers[i]->GetName()] = i;
		}
	}
	{
		RegisterAction("viewfps");
		RegisterAction("viewta");
		RegisterAction("viewspring");
		RegisterAction("viewrot");
		RegisterAction("viewfree");
		RegisterAction("viewov");
		RegisterAction("viewtaflip");

		RegisterAction("toggleoverview");
		RegisterAction("togglecammode");

		RegisterAction("viewsave");
		RegisterAction("viewload");

		RegisterAction("camtimefactor");
		RegisterAction("camtimeexponent");

		SortRegisteredActions();
	}

	configHandler->NotifyOnChange(this, {"CamModeName", "WindowedEdgeMove", "FullscreenEdgeMove", "CamTimeFactor", "CamTimeExponent", "CamTransitionMode", "CamSpringHalflife"});

	{
		camTransState.startFOV  = 90.0f;
		camTransState.timeStart =  0.0f;
		camTransState.timeEnd   =  0.0f;
	}

	ConfigNotify("", "");

	SetCameraMode(configHandler->GetString("CamModeName"));

	for (CCameraController* cc: camControllers) {
		cc->Update();
	}
}

void CCameraHandler::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	KillControllers();
}


void CCameraHandler::InitControllers()
{
	RECOIL_DETAILED_TRACY_ZONE;
	static_assert(sizeof(        CFPSController) <= sizeof(camControllerMem[CAMERA_MODE_FIRSTPERSON]), "");
	static_assert(sizeof(   COverheadController) <= sizeof(camControllerMem[CAMERA_MODE_OVERHEAD   ]), "");
	static_assert(sizeof(     CSpringController) <= sizeof(camControllerMem[CAMERA_MODE_SPRING     ]), "");
	static_assert(sizeof(CRotOverheadController) <= sizeof(camControllerMem[CAMERA_MODE_ROTOVERHEAD]), "");
	static_assert(sizeof(       CFreeController) <= sizeof(camControllerMem[CAMERA_MODE_FREE       ]), "");
	static_assert(sizeof(   COverviewController) <= sizeof(camControllerMem[CAMERA_MODE_OVERVIEW   ]), "");

	// FPS camera must always be the first one in the list
	camControllers[CAMERA_MODE_FIRSTPERSON] = new (camControllerMem[CAMERA_MODE_FIRSTPERSON])         CFPSController();
	camControllers[CAMERA_MODE_OVERHEAD   ] = new (camControllerMem[CAMERA_MODE_OVERHEAD   ])    COverheadController();
	camControllers[CAMERA_MODE_SPRING     ] = new (camControllerMem[CAMERA_MODE_SPRING     ])      CSpringController();
	camControllers[CAMERA_MODE_ROTOVERHEAD] = new (camControllerMem[CAMERA_MODE_ROTOVERHEAD]) CRotOverheadController();
	camControllers[CAMERA_MODE_FREE       ] = new (camControllerMem[CAMERA_MODE_FREE       ])        CFreeController();
	camControllers[CAMERA_MODE_OVERVIEW   ] = new (camControllerMem[CAMERA_MODE_OVERVIEW   ])    COverviewController();
}

void CCameraHandler::KillControllers()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (camControllers[0] == nullptr)
		return;

	SetCameraMode(CAMERA_MODE_DUMMY);

	for (unsigned int i = 0; i < CAMERA_MODE_DUMMY; i++) {
		spring::SafeDestruct(camControllers[i]);
		std::memset(camControllerMem[i], 0, sizeof(camControllerMem[i]));
	}

	assert(camControllers[0] == nullptr);
}

void CCameraHandler::ConfigNotify(const std::string& key, const std::string& value)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (key == "CamModeName") {
		SetCameraMode(configHandler->GetString("CamModeName"));
	} else {
		currCamTransitionNum = configHandler->GetInt("CamTransitionMode");

		windowedEdgeMove   = configHandler->GetBool("WindowedEdgeMove");
		fullscreenEdgeMove = configHandler->GetBool("FullscreenEdgeMove");

		camTransState.timeFactor = configHandler->GetFloat("CamTimeFactor");
		camTransState.timeExponent = configHandler->GetFloat("CamTimeExponent");
		camTransState.halflife = configHandler->GetFloat("CamSpringHalflife");

		camTransState.lastTime = spring_gettime().toMilliSecsf();
	}
}

void CCameraHandler::UpdateController(CPlayer* player, bool fpsMode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController& camCon = GetCurrentController();
	FPSUnitController& fpsCon = player->fpsController;

	const bool fusEdgeMove = ( globalRendering->fullScreen && fullscreenEdgeMove);
	const bool winEdgeMove = (!globalRendering->fullScreen && windowedEdgeMove);

	// We have to update transition both before and after updating the controller:
	// before: if the controller makes a new begincam, it needs to take into account previous transitions
	// after: apply changes made by the controller with 0 time transition.
	if (currCamTransitionNum == CAMERA_TRANSITION_MODE_EXP_DECAY) {
		UpdateTransition();
	}

	if (fpsCon.oldDCpos != ZeroVector) {
		camCon.SetPos(fpsCon.oldDCpos);
		fpsCon.oldDCpos = ZeroVector;
	}

	if (!fpsMode)
		UpdateController(camCon, true, true, fusEdgeMove || winEdgeMove);

	camCon.Update();

	UpdateTransition();
}

void CCameraHandler::UpdateController(CCameraController& camCon, bool keyMove, bool wheelMove, bool edgeMove)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (keyMove) {
		// NOTE: z-component contains speed scaling factor, xy is movement
		const float3 camMoveVector = camera->GetMoveVectorFromState(true);

		// key scrolling
		if ((camMoveVector * XYVector).SqLength() > 0.0f) {
			if (camCon.DisableTrackingByKey())
				unitTracker.Disable();

			camCon.KeyMove(camMoveVector);
		}
	}

	if (edgeMove) {
		const float3 camMoveVector = camera->GetMoveVectorFromState(false);

		// screen edge scrolling
		if ((camMoveVector * XYVector).SqLength() > 0.0f) {
			unitTracker.Disable();
			camCon.ScreenEdgeMove(camMoveVector);
		}
	}

	if (wheelMove) {
		// mouse wheel zoom
		float mouseWheelDist = 0.0f;
		mouseWheelDist +=  float(camera->GetMovState()[CCamera::MOVE_STATE_UP]);
		mouseWheelDist += -float(camera->GetMovState()[CCamera::MOVE_STATE_DWN]);
		mouseWheelDist *= 0.2f * globalRendering->lastFrameTime;

		if (mouseWheelDist == 0.0f)
			return;

		camCon.MouseWheelMove(mouseWheelDist);
	}
}

void CameraTransitionExpDecay(const CCameraController* currCam, CCameraHandler::CamTransitionState& camTransState, float nsecs)
{
	nsecs = std::max(nsecs, 0.0f) * camTransState.timeFactor;

	// calculate when transition should end based on duration in seconds
	if (camera->useInterpolate == 0) { // old
		camTransState.timeStart = globalRendering->lastFrameStart.toMilliSecsf();
	}
	if (camera->useInterpolate > 0) {
		camTransState.timeStart = globalRendering->lastSwapBuffersEnd.toMilliSecsf() + 1000.0f / std::fmax(globalRendering->FPS, 1.0f);
	}
	camTransState.timeEnd   = camTransState.timeStart + nsecs * 1000.0f;

	camTransState.startPos = camera->GetPos();
	camTransState.startRot = camera->GetRot();
	camTransState.startFOV = camera->GetVFOV();
}

void CameraTransitionTimedSpringDampened(const CCameraController* currCam, CCameraHandler::CamTransitionState& camTransState, float nsecs)
{
	if (nsecs == 0.0f) {
		camera->SetPos(currCam->GetPos());
		camera->SetRot(currCam->GetRot());
		camera->SetVFOV(currCam->GetFOV());
	} else if (nsecs > 0.0f) {
		camTransState.timeEnd = nsecs * 1000.0f * camTransState.timeFactor;
		camTransState.timeStart = nsecs * 1000.0f * camTransState.timeFactor;
	}
}

void CameraTransitionSpringDampened(const CCameraController* currCam, CCameraHandler::CamTransitionState& camTransState, float nsecs)
{
	if (nsecs == 0.0f) {
		camera->SetPos(currCam->GetPos());
		camera->SetRot(currCam->GetRot());
		camera->SetVFOV(currCam->GetFOV());
	}
}


static constexpr decltype(&CameraTransitionExpDecay) cameraTransitionFunction[] = {
	CameraTransitionExpDecay,
	CameraTransitionSpringDampened,
	CameraTransitionTimedSpringDampened,
	CameraTransitionSpringDampened, // lerp smoothed is same
};

void CCameraHandler::CameraTransition(float nsecs)
{
	RECOIL_DETAILED_TRACY_ZONE;
	cameraTransitionFunction[currCamTransitionNum](camControllers[currCamCtrlNum], camTransState, nsecs);
}

void UpdateTransitionExpDecay(const CCameraController* currCam, CCameraHandler::CamTransitionState& camTransState)
{
	RECOIL_DETAILED_TRACY_ZONE;
	camTransState.tweenPos = currCam->GetPos();
	camTransState.tweenRot = currCam->GetRot();
	camTransState.tweenFOV = currCam->GetFOV();

	int vsync = configHandler->GetInt("VSync");
	float transTime = globalRendering->lastFrameStart.toMilliSecsf();
	float lastswaptime = globalRendering->lastSwapBuffersEnd.toMilliSecsf();
	float drawFPS = std::fmax(globalRendering->FPS, 1.0f); // this is probably much better


	if (vsync == 1 && camera->useInterpolate > 0) {
		transTime = lastswaptime;
		transTime = globalRendering->lastSwapBuffersEnd.toMilliSecsf() + 1000.0f / drawFPS;
	}

	float timeRatio = (camTransState.timeEnd - camTransState.timeStart != 0.0f) ?
		std::fmax(0.0f, (camTransState.timeEnd - transTime) / (camTransState.timeEnd - camTransState.timeStart)) :
		0.0f;

	float tweenFact = 1.0f - math::pow(timeRatio, camTransState.timeExponent);


	if (vsync == 1 && camera->useInterpolate == 1) {
		tweenFact = 1.0f - timeRatio;
	}
	if (vsync == 0 && camera->useInterpolate == 1){
		tweenFact = 0.08f * (camTransState.timeEnd - camTransState.timeStart) / drawFPS ;// should be 0.25 at 120ms deltat and 60hz
	}
	/*
	LOG_L(L_INFO, "CamUpdate: %.3f, 1/FPS = %.4f, DF=%d, timeRatio = %.3f, tween = %.3f, swap = %.3f i=%d", globalRendering->lastFrameStart.toMilliSecsf(), drawFPS, globalRendering->drawFrame, timeRatio, tweenFact, lastswaptime, useInterpolate);
	LOG_L(L_INFO, "CamEnd: %.3f, CamStart = %.3f, deltat = %.3f CamExp = %.3f",
		camTransState.timeEnd,
		camTransState.timeStart,
		camTransState.timeEnd - camTransState.timeStart,
		camTransState.timeExponent
		);
	LOG_L(L_INFO, "CSX = %.2f, CSZ = %.2f, CTX = %.2f CTZ = %.2f", camTransState.startPos.x, camTransState.startPos.z, camTransState.tweenPos.x, camTransState.tweenPos.z );
	*/
	if (transTime >= camTransState.timeEnd) {
		camera->SetPos(camTransState.tweenPos);
		camera->SetRot(camTransState.tweenRot);
		camera->SetVFOV(camTransState.tweenFOV);
		camera->Update();
		return;
	}

	if (camTransState.timeEnd <= camTransState.timeStart)
		return;

	camera->SetPos(mix(camTransState.startPos, camTransState.tweenPos, tweenFact));
	camera->SetRot(mixRotation(camTransState.startRot, camTransState.tweenRot, tweenFact));
	camera->SetVFOV(mix(camTransState.startFOV, camTransState.tweenFOV, tweenFact));
	camera->Update();
}

void UpdateTransitionLerpSmoothed(const CCameraController* currCam, CCameraHandler::CamTransitionState& camTransState)
{
	float3 currentPos = camera->GetPos();
	float3 currentRot = camera->GetRot();
	float currentFov = camera->GetVFOV();

	float3 targetPos = currCam->GetPos();
	float3 targetRot = currCam->GetRot();
	float targetFov = currCam->GetFOV();
	float3 goalRot{};

	float currTime = spring_gettime().toMilliSecsf();
	float dt = currTime - camTransState.lastTime;
	camTransState.lastTime = currTime;

	float decay = camTransState.halflife / 1000.0f;
	expDecay(currentPos, targetPos, decay, dt);
	expDecay(currentRot, targetRot, decay, dt);
	expDecay(currentFov, targetFov, decay, dt);
	camera->SetPos(currentPos);
	camera->SetRot(currentRot);
	camera->SetVFOV(currentFov);
	camera->Update();
}

void UpdateTransitionTimedSpringDampened(const CCameraController* currCam, CCameraHandler::CamTransitionState& camTransState)
{
	float3 currentPos = camera->GetPos();
	float3 currentRot = camera->GetRot();
	float currentFov = camera->GetVFOV();

	float3 targetPos = currCam->GetPos();
	float3 targetRot = currCam->GetRot();
	float targetFov = currCam->GetFOV();

	float currTime = spring_gettime().toMilliSecsf();
	float dt = currTime - camTransState.lastTime;
	camTransState.lastTime = currTime;
	camTransState.timeEnd -= dt;
	camTransState.timeEnd = std::max(camTransState.timeEnd, 0.0f);
	camTransState.timeStart -= dt;
	camTransState.timeStart = std::max(camTransState.timeStart, 0.0f);

	if(currentPos.equals(targetPos)	&& currentRot.equals(targetRot)	&& currentFov == targetFov) {
		return;
	}

	float damping = spring_damper_damping(camTransState.halflife);
	float eydt = spring_damper_eydt(damping, dt);

	timed_spring_damper_exact_vector(currentPos, camTransState.posVelocity, camTransState.startPos,
		targetPos, camTransState.timeEnd, camTransState.halflife, damping, eydt, dt);
	timed_spring_damper_exact_vector(currentRot, camTransState.rotVelocity, camTransState.startRot,
		targetRot, camTransState.timeStart, camTransState.halflife, damping, eydt, dt);
	simple_spring_damper_exact(currentFov, camTransState.fovVelocity, targetFov, damping, eydt, dt);

	camera->SetPos(currentPos);
	camera->SetRot(currentRot);
	camera->SetVFOV(currentFov);
	camera->Update();
}

void UpdateTransitionSpringDampened(const CCameraController* currCam, CCameraHandler::CamTransitionState& camTransState){
	float3 currentPos = camera->GetPos();
	float3 currentRot = camera->GetRot();
	float currentFov = camera->GetVFOV();

	float3 targetPos = currCam->GetPos();
	float3 targetRot = currCam->GetRot();
	float targetFov = currCam->GetFOV();

	float currTime = spring_gettime().toMilliSecsf();
	float dt = currTime - camTransState.lastTime;
	camTransState.lastTime = currTime;

	if(currentPos.equals(targetPos)	&& currentRot.equals(targetRot)	&& currentFov == targetFov) {
		return;
	}

	float damping = spring_damper_damping(camTransState.halflife);
	float eydt = spring_damper_eydt(damping, dt);

	simple_spring_damper_exact_vector(currentPos, camTransState.posVelocity, targetPos, damping, eydt, dt);
	simple_spring_damper_exact_vector(currentRot, camTransState.rotVelocity, targetRot, damping, eydt, dt);
	simple_spring_damper_exact(currentFov, camTransState.fovVelocity, targetFov, damping, eydt, dt);
	// LOG_L(L_INFO, "tweenfact %0.3f, %0.3f, %0.3f", currentRot.y, targetRot.y, camTransState.rotVelocity.y);
	camera->SetPos(currentPos);
	camera->SetRot(currentRot);
	camera->SetVFOV(currentFov);
	camera->Update();
}

static constexpr decltype(&UpdateTransitionExpDecay) cameraUpdateTransitionFunction[] = {
	UpdateTransitionExpDecay,
	UpdateTransitionSpringDampened,
	UpdateTransitionTimedSpringDampened,
	UpdateTransitionLerpSmoothed,
};

void CCameraHandler::UpdateTransition()
{
	RECOIL_DETAILED_TRACY_ZONE;
	cameraUpdateTransitionFunction[currCamTransitionNum](camControllers[currCamCtrlNum], camTransState);
}

void CCameraHandler::SetCameraMode(unsigned int newMode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const unsigned int oldMode = currCamCtrlNum;

	if ((newMode >= camControllers.size()) || (newMode == oldMode))
		return;

	// switching {from, to} the dummy only happens on {init, kill}
	// in either case an interpolated transition is not necessary
	if (oldMode == CAMERA_MODE_DUMMY || newMode == CAMERA_MODE_DUMMY) {
		currCamCtrlNum = newMode;
		return;
	}
	CameraTransition(1.0f);

	CCameraController* oldCamCtrl = camControllers[                 oldMode];
	CCameraController* newCamCtrl = camControllers[currCamCtrlNum = newMode];

	// clamp rotations so that the camera doesnt spin excessively to get to the new rotation
	camera->SetRot(ClampRadPrincipal(camera->GetRot()));
	oldCamCtrl->SetRot(ClampRadPrincipal(oldCamCtrl->GetRot()));

	newCamCtrl->SwitchTo(oldCamCtrl);
	newCamCtrl->Update();
}

void CCameraHandler::SetCameraMode(const std::string& modeName)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int modeNum = (!modeName.empty())? GetModeIndex(modeName): configHandler->GetInt("CamMode");

	// do nothing if the name is not matched
	if (modeNum < 0)
		return;

	SetCameraMode(modeNum);
}


int CCameraHandler::GetModeIndex(const std::string& name) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = nameModeMap.find(name);

	if (it != nameModeMap.end())
		return it->second;

	return -1;
}


void CCameraHandler::PushMode()
{
	RECOIL_DETAILED_TRACY_ZONE;
	controllerStack.push_back(GetCurrentControllerNum());
}

void CCameraHandler::PopMode()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (controllerStack.empty())
		return;

	SetCameraMode(controllerStack.back());
	controllerStack.pop_back();
}


void CCameraHandler::ToggleState()
{
	RECOIL_DETAILED_TRACY_ZONE;
	unsigned int newMode = (currCamCtrlNum + 1) % camControllers.size();
	unsigned int numTries = 0;

	const unsigned int maxTries = camControllers.size() - 1;

	while ((numTries++ < maxTries) && !camControllers[newMode]->enabled) {
		newMode = (newMode + 1) % camControllers.size();
	}

	SetCameraMode(newMode);
}


void CCameraHandler::ToggleOverviewCamera()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CameraTransition(1.0f);

	if (controllerStack.empty()) {
		PushMode();
		SetCameraMode(CAMERA_MODE_OVERVIEW);
	} else {
		PopMode();
	}
}


void CCameraHandler::SaveView(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (name.empty())
		return;

	ViewData& vd = viewDataMap[name];
	vd["mode"] = currCamCtrlNum;

	camControllers[currCamCtrlNum]->GetState(vd);
}

bool CCameraHandler::LoadView(const std::string& name)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (name.empty())
		return false;

	const auto it = viewDataMap.find(name);

	if (it == viewDataMap.end())
		return false;

	const ViewData& saved = it->second;

	ViewData current;
	GetState(current);

	if (saved == current) {
		// load a view twice to return to old settings
		// safety: should not happen, but who knows?
		if (name != "__old_view")
			return LoadView("__old_view");

		return false;
	}

	if (name != "__old_view")
		SaveView("__old_view");

	return LoadViewData(saved);
}


void CCameraHandler::GetState(CCameraController::StateMap& sm) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	sm.clear();
	sm["mode"] = currCamCtrlNum;

	camControllers[currCamCtrlNum]->GetState(sm);
}

CCameraController::StateMap CCameraHandler::GetState() const
{
	RECOIL_DETAILED_TRACY_ZONE;
	CCameraController::StateMap sm;
	GetState(sm);
	return sm;
}

bool CCameraHandler::SetState(const CCameraController::StateMap& sm)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = sm.find("mode");

	if (it != sm.cend()) {
		const unsigned int camMode = it->second;
		const unsigned int oldMode = currCamCtrlNum;

		if (camMode >= camControllers.size())
			return false;

		if (camMode != currCamCtrlNum)
			camControllers[currCamCtrlNum = camMode]->SwitchTo(camControllers[oldMode]);
	}

	const bool result = camControllers[currCamCtrlNum]->SetState(sm);
	camControllers[currCamCtrlNum]->Update();
	return result;
}


void CCameraHandler::PushAction(const Action& action)
{
	RECOIL_DETAILED_TRACY_ZONE;
	switch (hashString(action.command.c_str())) {
		case hashString("viewfps"): {
			SetCameraMode(CAMERA_MODE_FIRSTPERSON);
		} break;
		case hashString("viewta"): {
			SetCameraMode(CAMERA_MODE_OVERHEAD);
		} break;
		case hashString("viewspring"): {
			SetCameraMode(CAMERA_MODE_SPRING);
		} break;
		case hashString("viewrot"): {
			SetCameraMode(CAMERA_MODE_ROTOVERHEAD);
		} break;
		case hashString("viewfree"): {
				SetCameraMode(CAMERA_MODE_FREE);
		} break;
		case hashString("viewov"): {
			SetCameraMode(CAMERA_MODE_OVERVIEW);
		} break;

		case hashString("viewtaflip"): {
			COverheadController* ohCamCtrl = dynamic_cast<COverheadController*>(camControllers[CAMERA_MODE_OVERHEAD]);

			if (ohCamCtrl != nullptr) {
				if (!action.extra.empty()) {
					ohCamCtrl->flipped = !!atoi(action.extra.c_str());
				} else {
					ohCamCtrl->flipped = !ohCamCtrl->flipped;
				}

				ohCamCtrl->Update();
			}
		} break;

		case hashString("viewsave"): {
			if (!action.extra.empty()) {
				SaveView(action.extra);
				LOG("[CamHandler::%s] saved view \"%s\"", __func__, action.extra.c_str());
			}
		} break;
		case hashString("viewload"): {
			if (LoadView(action.extra))
				LOG("[CamHandler::%s] loaded view \"%s\"", __func__, action.extra.c_str());

		} break;

		case hashString("toggleoverview"): {
			ToggleOverviewCamera();
		} break;
		case hashString("togglecammode"): {
			ToggleState();
		} break;

		case hashString("camtimefactor"): {
			if (!action.extra.empty())
				camTransState.timeFactor = std::atof(action.extra.c_str());

			LOG("[CamHandler::%s] set transition-time factor to %f", __func__, camTransState.timeFactor);
		} break;
		case hashString("camtimeexponent"): {
			if (!action.extra.empty())
				camTransState.timeExponent = std::atof(action.extra.c_str());

			LOG("[CamHandler::%s] set transition-time exponent to %f", __func__, camTransState.timeExponent);
		} break;
	}
}


bool CCameraHandler::LoadViewData(const ViewData& vd)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (vd.empty())
		return false;

	const auto it = vd.find("mode");

	if (it != vd.cend()) {
		const unsigned int camMode = it->second;
		const unsigned int curMode = currCamCtrlNum;

		if (camMode >= camControllers.size())
			return false;

		if (camMode != currCamCtrlNum) {
			CameraTransition(1.0f);
			camControllers[currCamCtrlNum = camMode]->SwitchTo(camControllers[curMode], camMode != curMode);
		}
	}

	return camControllers[currCamCtrlNum]->SetState(vd);
}

