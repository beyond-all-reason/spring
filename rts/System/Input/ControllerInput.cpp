/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ControllerInput.h"
#include "InputHandler.h"

#include <SDL.h>
#include <functional>

#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/EventHandler.h"

CONFIG(bool, JoystickEnabled).defaultValue(true).headlessValue(false);
CONFIG(int, JoystickUse).defaultValue(0);

ControllerInput* controllerInput = nullptr;

ControllerInput::ControllerInput()
{
	inputCon = input.AddHandler(std::bind(&ControllerInput::HandleSDLControllerEvent, this, std::placeholders::_1));
}

ControllerInput::~ControllerInput()
{
	DisconnectControllers();

	connectedControllers.clear();

	if (inputCon.connected())
		inputCon.disconnect();
}

void ControllerInput::InitStatic()
{
	assert(controllerInput == nullptr);

	const bool useJoystick = configHandler->GetBool("JoystickEnabled");
	//if (!useJoystick)
	//	return;

	const int err = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
	if (err == -1) {
		LOG_L(L_ERROR, "Could not initialise joystick subsystem: %s", SDL_GetError());
		return;
	};

	controllerInput = new ControllerInput();
}

void ControllerInput::KillStatic()
{
	spring::SafeDelete(controllerInput);

	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

bool ControllerInput::HandleSDLControllerEvent(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_CONTROLLERAXISMOTION:
		{
			LOG("[SpringApp::%s] Controller %d -> AXISMOTION: %s | %d",  __func__, event.cdevice.which, SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button), event.caxis.value);
			eventHandler.ControllerState("AxisMotion", event.cdevice.which, event.caxis.axis, event.caxis.value);
			break;
		}
		case SDL_CONTROLLERBUTTONDOWN:
		{
			LOG("[SpringApp::%s] Controller %d -> BUTTONDOWN: %s | %d",  __func__, event.cdevice.which, SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button), event.cbutton.state);
			eventHandler.ControllerState("ButtonDown", event.cdevice.which, event.cbutton.button, event.cbutton.state);
			break;
		}
		case SDL_CONTROLLERBUTTONUP:
		{
			LOG("[SpringApp::%s] Controller %d -> BUTTONUP: %s | %d",  __func__, event.cdevice.which, SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button), event.cbutton.state);
			eventHandler.ControllerState("ButtonUp", event.cdevice.which, event.cbutton.button, event.cbutton.state);
			break;
		}
		case SDL_CONTROLLERDEVICEADDED:
		{
			LOG_L(L_ERROR, "[SpringApp::%s] Controller %d -> DEVICEADDED",  __func__, event.cdevice.which);

			eventHandler.ControllerDevice("Added", event.cdevice.which);
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
			LOG_L(L_ERROR, "[SpringApp::%s] Controller %d -> DEVICEREMOVED: %s",  __func__, event.cdevice.which, SDL_GameControllerNameForIndex(event.cdevice.which));

			eventHandler.ControllerDevice("Removed", event.cdevice.which);
			break;
		default:
		{
		}
	}
	return false;
}

bool ControllerInput::DisconnectController(int instanceId) {
	return DisconnectController(instanceId, GetConnectedController(instanceId));
}

bool ControllerInput::DisconnectController(int instanceId, SDL_GameController* controller) {
	if (controller == nullptr)
		return false;

	SDL_GameControllerClose(controller);

	connectedControllers.erase(instanceId);

	eventHandler.ControllerDevice("Disconnected", instanceId);

	LOG_L(L_ERROR, "[SpringApp::%s] Controller %d -> Disconnected",  __func__, instanceId);

	return true;
}

void ControllerInput::DisconnectControllers()
{
	for (auto controllerPair : connectedControllers)
		DisconnectController(controllerPair.first, controllerPair.second);

	connectedControllers.clear();
}

SDL_GameController* ControllerInput::ConnectController(int deviceId, int& instanceId) {
	if (!SDL_IsGameController(deviceId))
		return nullptr;

	SDL_GameController* controller = SDL_GameControllerOpen(deviceId);

	if (controller == NULL)
		return nullptr;

	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);

	if (joystick == NULL)
		return nullptr;

	instanceId = SDL_JoystickInstanceID(joystick);

	connectedControllers.insert(instanceId, controller);

	LOG_L(L_ERROR, "[SpringApp::%s] Controller %d -> Connected: %d",  __func__, deviceId, instanceId);
	eventHandler.ControllerDevice("Connected", instanceId);

	return controller;
}

ControllerInput::ControllerSet ControllerInput::GetConnectedControllers()
{
	ControllerInput::ControllerSet controllerSet;

	controllerSet.reserve(connectedControllers.size());

	for (auto controllerPair : connectedControllers)
		controllerSet.insert(controllerPair.first);

	return controllerSet;
}

SDL_GameController* ControllerInput::GetConnectedController(int instanceId)
{
	auto it = connectedControllers.find(instanceId);

	if (it == connectedControllers.end())
		return nullptr;

	return it->second;
}
