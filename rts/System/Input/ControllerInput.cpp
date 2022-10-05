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

std::string ControllerInput::GetDeviceGUID(int deviceId)
{
	return GetGUIDString(SDL_JoystickGetDeviceGUID(deviceId));
}

std::string ControllerInput::GetControllerGUID(int deviceId)
{
	const auto controller = SDL_GameControllerFromInstanceID(deviceId);
	const auto joystick = SDL_GameControllerGetJoystick(controller);
	return GetGUIDString(SDL_JoystickGetGUID(joystick));
}

std::string ControllerInput::GetMappingForGUID(std::string guid)
{
	const auto joyGUID = SDL_JoystickGetGUIDFromString(guid.c_str());
	return SDL_GameControllerMappingForGUID(joyGUID);
}

std::string ControllerInput::GetGUIDString(SDL_JoystickGUID joyGUID)
{
	char guid[64];
	SDL_JoystickGetGUIDString(joyGUID, guid, sizeof(guid));

	return guid;
}

bool ControllerInput::HandleSDLControllerEvent(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_CONTROLLERAXISMOTION:
		{
			eventHandler.ControllerAxisMotion(event.cdevice.which, event.caxis.axis, event.caxis.value);
			break;
		}
		case SDL_CONTROLLERBUTTONDOWN:
		{
			eventHandler.ControllerButtonDown(event.cdevice.which, event.cbutton.button, event.cbutton.state);
			break;
		}
		case SDL_CONTROLLERBUTTONUP:
		{
			eventHandler.ControllerButtonUp(event.cdevice.which, event.cbutton.button, event.cbutton.state);
			break;
		}
		case SDL_CONTROLLERDEVICEADDED:
		{
			eventHandler.ControllerAdded(event.cdevice.which);
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			eventHandler.ControllerRemoved(event.cdevice.which);
			break;
		}
		case SDL_CONTROLLERDEVICEREMAPPED:
		{
			eventHandler.ControllerRemapped(event.cdevice.which);
			break;
		}
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

	eventHandler.ControllerDisconnected(instanceId);

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

	eventHandler.ControllerConnected(instanceId);

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
