/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
#include "UI/UnitTracker.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/VerticalSync.h"
#include "System/SpringMath.h"
#include "System/SafeUtil.h"
#include "System/StringHash.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "Game/GlobalUnsynced.h"

#define TRACY_CAMERA

#ifdef TRACY_CAMERA
	#include <tracy/Tracy.hpp>
	#include <common/TracyColor.hpp>
	const char* const tracyCameraTimeRatio = "CameraTimeRatio";
	const char* const tracyCameraTransitionDuration = "CameraTransitionDuration";
	const char* const tracyCameraTweenFact = "CameraTweenFact";
	const char* const tracyCameraDrawFPS = "CameraDrawFPS";
	const char* const tracyCameraTimeRatioSmooth = "CameraTimeRatioSmooth";
	const char* const tracyCameraTimeOffset = "CameraTimeOffset";
	const char* const tracyAvgDrawTime = "AvgDrawTime";
	const char* const tracyCamStart= "CamStart";
	const char* const tracyCamEnd = "CamEnd";
#endif


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

CONFIG(float, CamTimeFactor)
	.defaultValue(1.0f)
	.minimumValue(0.0f)
	.description("Scales the speed of camera transitions, e.g. zooming or position change.");

CONFIG(float, CamTimeExponent)
	.defaultValue(4.0f)
	.minimumValue(0.0f)
	.description("Camera transitions happen at lerp(old, new, timeNorm ^ CamTimeExponent).");

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
	camControllers.fill(nullptr);
	camControllers[CAMERA_MODE_DUMMY] = new (camControllerMem[CAMERA_MODE_DUMMY]) CDummyController();
	#ifdef TRACY_CAMERA
		TracyPlotConfig(tracyCameraTimeRatio, tracy::PlotFormatType::Number, true, false, tracy::Color::White);
		TracyPlotConfig(tracyCameraTransitionDuration, tracy::PlotFormatType::Number, true, false, tracy::Color::White);
		TracyPlotConfig(tracyCameraTweenFact, tracy::PlotFormatType::Number, true, false, tracy::Color::White);
		TracyPlotConfig(tracyCameraDrawFPS, tracy::PlotFormatType::Number, true, false, tracy::Color::White);
		TracyPlotConfig(tracyCameraTimeRatioSmooth, tracy::PlotFormatType::Number, false, false, tracy::Color::White);
		TracyPlotConfig(tracyCameraTimeOffset, tracy::PlotFormatType::Number, true, false, tracy::Color::White);
		TracyPlotConfig(tracyAvgDrawTime, tracy::PlotFormatType::Number, true, false, tracy::Color::White);
		TracyPlotConfig(tracyCamStart, tracy::PlotFormatType::Number, false, false, tracy::Color::White);
		TracyPlotConfig(tracyCamEnd, tracy::PlotFormatType::Number, false, false, tracy::Color::White);
	#endif
}

CCameraHandler::~CCameraHandler() {
	// regular controllers should already have been killed
	assert(camControllers[0] == nullptr);
	spring::SafeDestruct(camControllers[CAMERA_MODE_DUMMY]);
	std::memset(camControllerMem[CAMERA_MODE_DUMMY], 0, sizeof(camControllerMem[CAMERA_MODE_DUMMY]));
}


void CCameraHandler::Init()
{
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

	{
		camTransState.startFOV  = 90.0f;
		camTransState.timeStart =  0.0f;
		camTransState.timeEnd   =  0.0f;

		camTransState.timeFactor   = configHandler->GetFloat("CamTimeFactor");
		camTransState.timeExponent = configHandler->GetFloat("CamTimeExponent");
	}

	SetCameraMode(configHandler->GetString("CamModeName"));

	for (CCameraController* cc: camControllers) {
		cc->Update();
	}
}

void CCameraHandler::Kill()
{
	KillControllers();
}


void CCameraHandler::InitControllers()
{
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
	if (camControllers[0] == nullptr)
		return;

	SetCameraMode(CAMERA_MODE_DUMMY);

	for (unsigned int i = 0; i < CAMERA_MODE_DUMMY; i++) {
		spring::SafeDestruct(camControllers[i]);
		std::memset(camControllerMem[i], 0, sizeof(camControllerMem[i]));
	}

	assert(camControllers[0] == nullptr);
}


void CCameraHandler::UpdateController(CPlayer* player, bool fpsMode, bool fsEdgeMove, bool wnEdgeMove)
{
	#ifdef TRACY_CAMERA
		ZoneScopedN("CCameraHandler::UpdateController(CPlayer)"); 
	#endif
	CCameraController& camCon = GetCurrentController();
	FPSUnitController& fpsCon = player->fpsController;

	const bool fusEdgeMove = ( globalRendering->fullScreen && fsEdgeMove);
	const bool winEdgeMove = (!globalRendering->fullScreen && wnEdgeMove);

	// We have to update transition both before and after updating the controller:
	// before: if the controller makes a new begincam, it needs to take into account previous transitions
	// after: apply changes made by the controller with 0 time transition.
	UpdateTransition();

	if (fpsCon.oldDCpos != ZeroVector) {
		camCon.SetPos(fpsCon.oldDCpos);
		fpsCon.oldDCpos = ZeroVector;
	}

	// Note that the Controller is responsible for mouse and keyboard camera movements
	if (!fpsMode)
		UpdateController(camCon, true, true, fusEdgeMove || winEdgeMove);

	camCon.Update();

	UpdateTransition();
}


void CCameraHandler::UpdateController(CCameraController& camCon, bool keyMove, bool wheelMove, bool edgeMove)
{
	#ifdef TRACY_CAMERA
		ZoneScopedN("CCameraHandler::UpdateController(CCameraController)"); 
	#endif
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
// This function should be used by all functions that try to get a timing for interpolating camera positions.
// It returns, in milliseconds, the expected additional time of when the currently drawn frame will be displayed, 
// counting from the start of the update.
// Ponders into the orb.
float CCameraHandler::GetCameraTimeOffset(float unUsed){
	
	int vsync = verticalSync->GetInterval();
	float lastswaptime = globalRendering->lastSwapBuffersEnd.toMilliSecsf();
	float nowTime = globalRendering->lastFrameStart.toMilliSecsf();

	// This value reflects how much time we spent pretty much in SimFrame only, preceding this new frame
	float msSinceLastSwapBuffers = (globalRendering->lastFrameStart - globalRendering->lastSwapBuffersEnd).toMilliSecsf();
	float avgFrameTimeMs = 1000.0 / std::fmax(cameraExpSmoothFps, 1.0f);

	
	float approximateFrameCount = msSinceLastSwapBuffers / avgFrameTimeMs;

	// Approximate number of milliseconds spent in Draw
	float timeSpentInDraw = globalRendering->avgDrawFrameTime * cameraExpSmoothFps;
	//float 

	float timeOffset = 0;
	if (vsync == 0 ){ 
		// vsync off, special case 
		// kinda need to figure in Sim Load too
		//paused?

		
		approximateFrameCount = std::fmax(1.0, approximateFrameCount);
		timeOffset =  avgFrameTimeMs*approximateFrameCount;
	}else{ // vsync on
		approximateFrameCount = std::fmax(1.0, std::floor(approximateFrameCount + 0.5f));
		timeOffset =  avgFrameTimeMs*approximateFrameCount;
	}
	#ifdef TRACY_CAMERA
		TracyPlot(tracyCameraTimeOffset, approximateFrameCount);
		TracyPlot(tracyAvgDrawTime, timeSpentInDraw);
	#endif
	return timeOffset;
};

void CCameraHandler::CameraTransition(float nsecs)
{
	#ifdef TRACY_CAMERA
		ZoneScopedN("CCameraHandler::CameraTransition"); 
	#endif
	nsecs = std::max(nsecs, 0.0f) * camTransState.timeFactor;

	// calculate when transition should end based on duration in seconds
	// Note: Some games call Spring.SetCameraState(nil, nsecs(default 0!)) to override the camera into a much smoother one. 
	// This is partly due to the misunderstanding of the camtimeexponent, and camtimefactor configuration params
	// But also due to the fact that what they desire is a quick and linear initial transition, 
	// followed by a very slow movement to the desired position. 
	// This is often much more easily achieved by using a large CamTimeFactor + large CamTimeExponent.
	// However, camtimeFactor and camtimeExponent are not taken into account when doing keyboard pans, or rotates around world.
	// The default 0 arg means that if one wishes to add time to every Spring.SetCameraState, then it must be done there, instead of for every goddamned frame.

	if (camera->useInterpolate == 0) { // old
		camTransState.timeStart = globalRendering->lastFrameStart.toMilliSecsf();
	}
	if (camera->useInterpolate == 1) {
		camTransState.timeStart = globalRendering->lastSwapBuffersEnd.toMilliSecsf() + 1000.0f / std::fmax(globalRendering->FPS, 1.0f);
	}
	if (camera->useInterpolate == 2) {
		// lastFrameStart is actually the exact time this current update+draw frame started. 
		// This is imperfect, 
		camTransState.timeStart = globalRendering->lastFrameStart.toMilliSecsf();// + GetCameraTimeOffset(0.0);
	}
	if (globalRendering->luaTimeOffset) {
		camTransState.timeStart = globalRendering->lastSwapBuffersEnd.toMilliSecsf() + globalRendering->luaCameraTransitionTimeOffset;
		char tracybuf[128];
		sprintf(tracybuf, "CameraTransitionTimeOffset %f",camTransState.timeStart);
		TracyMessage(tracybuf, sizeof(tracybuf)); 
		#ifdef TRACY_TIMEOFFSET
			//TracyMessageL("luaCameraTransitionTimeOffset");
		#endif
	}

	TracyPlot(tracyCamStart, camTransState.timeStart);

	camTransState.timeEnd   = camTransState.timeStart + nsecs * 1000.0f;

	camTransState.startPos = camera->GetPos();
	camTransState.startRot = camera->GetRot();
	camTransState.startFOV = camera->GetVFOV();
}

void CCameraHandler::UpdateTransition()
{
	#ifdef TRACY_CAMERA
		ZoneScopedN("CCameraHandler::UpdateTransition"); // This helps us in checking if multiple camera commands trigger this.
	#endif
	camTransState.tweenPos = camControllers[currCamCtrlNum]->GetPos();
	camTransState.tweenRot = camControllers[currCamCtrlNum]->GetRot();
	camTransState.tweenFOV = camControllers[currCamCtrlNum]->GetFOV();

	globalRendering->lastFrameStart.toMicroSecsi();
	int vsync = configHandler->GetInt("VSync");
	float nowTime = globalRendering->lastFrameStart.toMilliSecsf(); // This is the timestamp of when the current draw frame was started. Woefully inaccurate due to the (float is woefully inaccurate here, considering that  game can easily go to 10K seconds, soooo.)
	float lastswaptime = globalRendering->lastSwapBuffersEnd.toMilliSecsf();
	float drawFPS = std::fmax(globalRendering->FPS, 1.0f); // this is probably much better



	// REFACTOR
	if(vsync == 0){
		if (camera->useInterpolate == 0){
		}
		else if (camera->useInterpolate == 1){
		}
		else if (camera->useInterpolate == 2){
		}

	}else{
		if (camera->useInterpolate == 0){
		}
		else if (camera->useInterpolate == 1){
		}
		else if (camera->useInterpolate == 2){
		}
	}


	// When vsync is on and camera interpolations is smoothed then look 1 frame into the future.
	// Instead, we should be looking 1 frame into the past!
	if (vsync == 1 && camera->useInterpolate > 0) {
		nowTime = lastswaptime + 1000.0f / drawFPS;
	}

	if (globalRendering->luaTimeOffset){
		nowTime = globalRendering->lastSwapBuffersEnd.toMilliSecsf() + globalRendering->luaCameraTimeOffset;
		#ifdef TRACY_TIMEOFFSET
			TracyMessageL("luaCameraTimeOffset");
		#endif
	}
	// Goes from 1 to 0 as a fraction of how much of the camera transition is remaining to be executed
	float timeRatio = 0.0;

	// The duration of the camera movement in milliseconds
	float cameraTransitionDuration = camTransState.timeEnd - camTransState.timeStart;
	if (cameraTransitionDuration > 0.0f){ // If its positive in duration, 
		timeRatio = std::clamp((camTransState.timeEnd - nowTime) / cameraTransitionDuration, 0.0f, 1.0f);
	}

	// Goes from 0.0 to 1.0 if input is also 0 to 1
	float tweenFact = 1.0f - math::pow(timeRatio, camTransState.timeExponent);
	
	if (!globalRendering->luaTimeOffset){
		if (vsync == 1 && camera->useInterpolate == 1) {
			tweenFact = 1.0f - timeRatio;
		}
		if (vsync == 0 && camera->useInterpolate == 1){
			// TODO COMPLETELY WRONG!
			//tweenFact = 0.08f * (cameraTransitionDuration) / drawFPS ;// should be 0.25 at 120ms deltat and 60hz
			tweenFact = 1.0f - timeRatio;
		}
	}



	#ifdef TRACY_CAMERA
		TracyPlot(tracyCameraTimeRatio, timeRatio);
		TracyPlot(tracyCameraTransitionDuration, cameraTransitionDuration);
		TracyPlot(tracyCameraTweenFact, tweenFact);
		TracyPlot(tracyCameraDrawFPS, drawFPS);
		TracyPlot(tracyCameraTimeRatioSmooth, timeRatio);
	#endif
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
	if (nowTime >= camTransState.timeEnd) {
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


void CCameraHandler::SetCameraMode(unsigned int newMode)
{
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

	newCamCtrl->SetPos(oldCamCtrl->SwitchFrom());
	newCamCtrl->SwitchTo(oldMode);
	newCamCtrl->Update();
}

void CCameraHandler::SetCameraMode(const std::string& modeName)
{
	const int modeNum = (!modeName.empty())? GetModeIndex(modeName): configHandler->GetInt("CamMode");

	// do nothing if the name is not matched
	if (modeNum < 0)
		return;

	SetCameraMode(modeNum);
}


int CCameraHandler::GetModeIndex(const std::string& name) const
{
	const auto it = nameModeMap.find(name);

	if (it != nameModeMap.end())
		return it->second;

	return -1;
}


void CCameraHandler::PushMode()
{
	controllerStack.push_back(GetCurrentControllerNum());
}

void CCameraHandler::PopMode()
{
	if (controllerStack.empty())
		return;

	SetCameraMode(controllerStack.back());
	controllerStack.pop_back();
}


void CCameraHandler::ToggleState()
{
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
	if (name.empty())
		return;

	ViewData& vd = viewDataMap[name];
	vd["mode"] = currCamCtrlNum;

	camControllers[currCamCtrlNum]->GetState(vd);
}

bool CCameraHandler::LoadView(const std::string& name)
{
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
	sm.clear();
	sm["mode"] = currCamCtrlNum;

	camControllers[currCamCtrlNum]->GetState(sm);
}

CCameraController::StateMap CCameraHandler::GetState() const
{
	CCameraController::StateMap sm;
	GetState(sm);
	return sm;
}

bool CCameraHandler::SetState(const CCameraController::StateMap& sm)
{
	const auto it = sm.find("mode");

	if (it != sm.cend()) {
		const unsigned int camMode = it->second;
		const unsigned int oldMode = currCamCtrlNum;

		if (camMode >= camControllers.size())
			return false;

		if (camMode != currCamCtrlNum)
			camControllers[currCamCtrlNum = camMode]->SwitchTo(oldMode);
	}

	const bool result = camControllers[currCamCtrlNum]->SetState(sm);
	camControllers[currCamCtrlNum]->Update();
	return result;
}


void CCameraHandler::PushAction(const Action& action)
{
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
			camControllers[currCamCtrlNum = camMode]->SwitchTo(curMode, camMode != curMode);
		}
	}

	return camControllers[currCamCtrlNum]->SetState(vd);
}

