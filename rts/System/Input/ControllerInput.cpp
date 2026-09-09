/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ControllerInput.h"
#include "InputHandler.h"

#include <SDL3/SDL.h>
#include <functional>

#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/SafeUtil.h"
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
}

void ControllerInput::InitStatic()
{
	assert(controllerInput == nullptr);

	const bool useJoystick = configHandler->GetBool("JoystickEnabled");
	if (!useJoystick)
		return;

	const bool err = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
	if (!err) {
		LOG_L(L_ERROR, "Could not initialise gamepad subsystem: %s", SDL_GetError());
		return;
	}

	controllerInput = new ControllerInput();
}

void ControllerInput::KillStatic()
{
	spring::SafeDelete(controllerInput);

	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

bool ControllerInput::HandleSDLControllerEvent(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		{
			LOG_L(L_DEBUG, "[SpringApp::%s] Controller %d -> AXISMOTION: %s | %d", __func__, event.gaxis.which, SDL_GetGamepadStringForAxis((SDL_GamepadAxis)event.gaxis.axis), event.gaxis.value);
			eventHandler.ControllerState("AxisMotion", event.gaxis.which, event.gaxis.axis, event.gaxis.value);
			break;
		}
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		{
			LOG_L(L_DEBUG, "[SpringApp::%s] Controller %d -> BUTTONDOWN: %s", __func__, event.gbutton.which, SDL_GetGamepadStringForButton((SDL_GamepadButton)event.gbutton.button));
			eventHandler.ControllerState("ButtonDown", event.gbutton.which, event.gbutton.button, 1);
			break;
		}
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
		{
			LOG_L(L_DEBUG, "[SpringApp::%s] Controller %d -> BUTTONUP: %s", __func__, event.gbutton.which, SDL_GetGamepadStringForButton((SDL_GamepadButton)event.gbutton.button));
			eventHandler.ControllerState("ButtonUp", event.gbutton.which, event.gbutton.button, 0);
			break;
		}
		case SDL_EVENT_GAMEPAD_ADDED:
		{
			LOG_L(L_INFO, "[SpringApp::%s] Controller %d -> ADDED", __func__, event.gdevice.which);
			eventHandler.ControllerDevice("Added", event.gdevice.which);
			break;
		}
		case SDL_EVENT_GAMEPAD_REMOVED:
		{
			LOG_L(L_INFO, "[SpringApp::%s] Controller %d -> REMOVED", __func__, event.gdevice.which);
			eventHandler.ControllerDevice("Removed", event.gdevice.which);
			DisconnectController(event.gdevice.which);
			break;
		}
		case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
		case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
		case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
		case SDL_EVENT_GAMEPAD_SENSOR_UPDATE:
		default:
		{
		}
	}
	return false;
}

bool ControllerInput::DisconnectController(int instanceId)
{
	return DisconnectController(instanceId, GetConnectedController(instanceId));
}

bool ControllerInput::DisconnectController(int instanceId, SDL_Gamepad* gamepad)
{
	if (gamepad == nullptr)
		return false;

	SDL_CloseGamepad(gamepad);

	connectedControllers.erase(instanceId);

	eventHandler.ControllerDevice("Disconnected", instanceId);

	LOG_L(L_INFO, "[SpringApp::%s] Controller %d -> Disconnected", __func__, instanceId);

	return true;
}

void ControllerInput::DisconnectControllers()
{
	for (auto& controllerPair : connectedControllers)
		DisconnectController(controllerPair.first, controllerPair.second);

	connectedControllers.clear();
}

SDL_Gamepad* ControllerInput::ConnectController(int deviceId, int& instanceId)
{
	if (!SDL_IsGamepad(deviceId))
		return nullptr;

	SDL_Gamepad* gamepad = SDL_OpenGamepad(deviceId);

	if (gamepad == nullptr)
		return nullptr;

	SDL_Joystick* joystick = SDL_GetGamepadJoystick(gamepad);

	if (joystick == nullptr)
		return nullptr;

	instanceId = SDL_GetJoystickID(joystick);

	connectedControllers[instanceId] = gamepad;

	LOG_L(L_INFO, "[SpringApp::%s] Controller %d -> Connected (instance %d): %s", __func__, deviceId, instanceId, SDL_GetGamepadName(gamepad));
	eventHandler.ControllerDevice("Connected", instanceId);

	return gamepad;
}

ControllerInput::ControllerSet ControllerInput::GetConnectedControllers()
{
	ControllerInput::ControllerSet controllerSet;

	controllerSet.reserve(connectedControllers.size());

	for (auto& controllerPair : connectedControllers)
		controllerSet.insert(controllerPair.first);

	return controllerSet;
}

SDL_Gamepad* ControllerInput::GetConnectedController(int instanceId)
{
	auto it = connectedControllers.find(instanceId);

	if (it == connectedControllers.end())
		return nullptr;

	return it->second;
}
