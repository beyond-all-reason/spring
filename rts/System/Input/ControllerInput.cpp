/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "ControllerInput.h"

#include "System/Input/InputHandler.h"
#include "System/Log/ILog.h"

#ifndef CONTROLLER_INPUT_LOG_EVENTS
#define CONTROLLER_INPUT_LOG_EVENTS 0
#endif

#ifndef HEADLESS
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_error.h>
#include <SDL_gamecontroller.h>
#include <SDL_joystick.h>
#endif

CControllerInput* controllerInput = nullptr;

CControllerInput* CControllerInput::GetInstance()
{
	if (controllerInput == nullptr) {
		controllerInput = new CControllerInput();
	}

	return controllerInput;
}

void CControllerInput::FreeInstance()
{
	delete controllerInput;
	controllerInput = nullptr;
}

#ifndef HEADLESS

std::vector<CControllerInput::ControllerState> CControllerInput::GetAvailableControllers() const
{
	std::vector<ControllerState> controllers;
	controllers.reserve(controllersByInstanceID.size());

	for (const auto& controllerIt : controllersByInstanceID) {
		controllers.push_back(controllerIt.second);
	}

	return controllers;
}

std::optional<CControllerInput::ControllerState> CControllerInput::GetControllerState(int instanceID) const
{
	const auto controllerIt = controllersByInstanceID.find(instanceID);
	if (controllerIt == controllersByInstanceID.end()) {
		return std::nullopt;
	}

	return controllerIt->second;
}

CControllerInput::CControllerInput()
{
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
		LOG_L(L_WARNING, "[ControllerInput] Failed to initialize SDL joystick/gamecontroller subsystem: %s", SDL_GetError());
	} else {
		LOG_L(L_INFO, "[ControllerInput] SDL joystick/gamecontroller subsystem initialized");
	}

	SDL_GameControllerEventState(SDL_ENABLE);
	SDL_JoystickEventState(SDL_ENABLE);

	inputCon = input.AddHandler([this](const SDL_Event& event) {
		return this->HandleSDLControllerEvent(event);
	});

	LOG_L(L_INFO, "[ControllerInput] SDL_NumJoysticks at init: %d", SDL_NumJoysticks());
	ScanExistingControllers();

	LOG_L(L_INFO, "[ControllerInput] Initialized SDL controller input handler");
}

CControllerInput::~CControllerInput()
{
	for (auto& controllerIt : controllersByInstanceID) {
		auto& state = controllerIt.second;

		if (state.gameController != nullptr) {
			SDL_GameControllerClose(state.gameController);
			state.gameController = nullptr;
		}
	}

	controllersByInstanceID.clear();

	LOG_L(L_INFO, "[ControllerInput] Shutting down SDL controller input handler");
}

bool CControllerInput::HandleSDLControllerEvent(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_CONTROLLERDEVICEADDED: {
			HandleDeviceAdded(event.cdevice.which);
		} break;

		case SDL_CONTROLLERDEVICEREMOVED: {
			HandleDeviceRemoved(event.cdevice.which);
		} break;

		case SDL_CONTROLLERDEVICEREMAPPED: {
			HandleDeviceRemapped(event.cdevice.which);
		} break;

		case SDL_CONTROLLERBUTTONDOWN: {
			HandleButtonDown(event.cbutton.which, event.cbutton.button, event.cbutton.state);
		} break;

		case SDL_CONTROLLERBUTTONUP: {
			HandleButtonUp(event.cbutton.which, event.cbutton.button, event.cbutton.state);
		} break;

		case SDL_CONTROLLERAXISMOTION: {
			HandleAxisMotion(event.caxis.which, event.caxis.axis, event.caxis.value);
		} break;

		default:
			break;
	}

	return false;
}

void CControllerInput::LogAvailableController(int deviceID) const
{
	if (SDL_IsGameController(deviceID)) {
		const char* name = SDL_GameControllerNameForIndex(deviceID);
		LOG_L(L_INFO, "[ControllerInput] SDL game controller available: deviceID=%d name=%s", deviceID, name != nullptr ? name : "unknown");
		return;
	}

	const char* name = SDL_JoystickNameForIndex(deviceID);
	LOG_L(L_INFO, "[ControllerInput] SDL joystick available but not game controller: deviceID=%d name=%s", deviceID, name != nullptr ? name : "unknown");
}

void CControllerInput::ScanExistingControllers()
{
	const int joystickCount = SDL_NumJoysticks();
	LOG_L(L_INFO, "[ControllerInput] Scanning existing SDL joysticks: count=%d", joystickCount);

	for (int deviceID = 0; deviceID < joystickCount; ++deviceID) {
		LogAvailableController(deviceID);

		if (SDL_IsGameController(deviceID)) {
			HandleDeviceAdded(deviceID);
			continue;
		}

		LOG_L(L_INFO, "[ControllerInput] Skipping existing non-game-controller device: deviceID=%d", deviceID);
	}
}

void CControllerInput::HandleDeviceAdded(int deviceID)
{
	LOG_L(L_INFO, "[ControllerInput] Controller device added: deviceID=%d", deviceID);
	LogAvailableController(deviceID);

	if (!SDL_IsGameController(deviceID)) {
		LOG_L(L_INFO, "[ControllerInput] Ignoring non-game-controller device: deviceID=%d", deviceID);
		return;
	}

	SDL_GameController* gameController = SDL_GameControllerOpen(deviceID);
	if (gameController == nullptr) {
		LOG_L(L_WARNING, "[ControllerInput] Failed to open SDL game controller: deviceID=%d error=%s", deviceID, SDL_GetError());
		return;
	}

	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(gameController);
	const int instanceID = joystick != nullptr ? SDL_JoystickInstanceID(joystick) : -1;

	if (instanceID < 0) {
		LOG_L(L_WARNING, "[ControllerInput] Failed to get joystick instance ID: deviceID=%d", deviceID);
		SDL_GameControllerClose(gameController);
		return;
	}

	TrackedControllerState state;
	state.deviceID = deviceID;
	state.instanceID = instanceID;

	const char* name = SDL_GameControllerName(gameController);
	state.name = name != nullptr ? name : "unknown";
	state.gameController = gameController;

	auto existingIt = controllersByInstanceID.find(instanceID);
	if (existingIt != controllersByInstanceID.end() && existingIt->second.gameController != nullptr) {
		SDL_GameControllerClose(existingIt->second.gameController);
	}

	controllersByInstanceID[instanceID] = state;

	LOG_L(L_INFO, "[ControllerInput] Controller connected: deviceID=%d instanceID=%d name=%s", deviceID, instanceID, state.name.c_str());
}

void CControllerInput::HandleDeviceRemoved(int instanceID)
{
	LOG_L(L_INFO, "[ControllerInput] Controller device removed: instanceID=%d", instanceID);

	auto it = controllersByInstanceID.find(instanceID);
	if (it == controllersByInstanceID.end()) {
		return;
	}

	LOG_L(L_INFO, "[ControllerInput] Removed tracked controller: instanceID=%d name=%s", instanceID, it->second.name.c_str());

	if (it->second.gameController != nullptr) {
		SDL_GameControllerClose(it->second.gameController);
		it->second.gameController = nullptr;
	}

	controllersByInstanceID.erase(it);
}

void CControllerInput::HandleDeviceRemapped(int instanceID)
{
	LOG_L(L_INFO, "[ControllerInput] Controller remapped: instanceID=%d", instanceID);
}

void CControllerInput::HandleButtonDown(int instanceID, int buttonID, std::uint8_t value)
{
	auto controllerIt = controllersByInstanceID.find(instanceID);
	if (controllerIt == controllersByInstanceID.end()) {
		return;
	}

	auto& state = controllerIt->second;

	if (buttonID >= 0 && buttonID < static_cast<int>(state.buttons.size())) {
		state.buttons[buttonID] = value;
	}

#if CONTROLLER_INPUT_LOG_EVENTS
	LOG_L(L_INFO, "[ControllerInput] ButtonDown: instanceID=%d buttonID=%d value=%u", instanceID, buttonID, static_cast<unsigned int>(value));
#endif
}

void CControllerInput::HandleButtonUp(int instanceID, int buttonID, std::uint8_t value)
{
	auto controllerIt = controllersByInstanceID.find(instanceID);
	if (controllerIt == controllersByInstanceID.end()) {
		return;
	}

	auto& state = controllerIt->second;

	if (buttonID >= 0 && buttonID < static_cast<int>(state.buttons.size())) {
		state.buttons[buttonID] = value;
	}

#if CONTROLLER_INPUT_LOG_EVENTS
	LOG_L(L_INFO, "[ControllerInput] ButtonUp: instanceID=%d buttonID=%d value=%u", instanceID, buttonID, static_cast<unsigned int>(value));
#endif
}

void CControllerInput::HandleAxisMotion(int instanceID, int axisID, std::int16_t value)
{
	auto controllerIt = controllersByInstanceID.find(instanceID);
	if (controllerIt == controllersByInstanceID.end()) {
		return;
	}

	auto& state = controllerIt->second;

	if (axisID >= 0 && axisID < static_cast<int>(state.axes.size())) {
		state.axes[axisID] = value;
	}

#if CONTROLLER_INPUT_LOG_EVENTS
	LOG_L(L_INFO, "[ControllerInput] AxisMotion: instanceID=%d axisID=%d value=%d", instanceID, axisID, static_cast<int>(value));
#endif
}

#else

CControllerInput::CControllerInput() = default;
CControllerInput::~CControllerInput() = default;

std::vector<CControllerInput::ControllerState> CControllerInput::GetAvailableControllers() const
{
	return {};
}

std::optional<CControllerInput::ControllerState> CControllerInput::GetControllerState(int) const
{
	return std::nullopt;
}

bool CControllerInput::HandleSDLControllerEvent(const SDL_Event&)
{
	return false;
}

void CControllerInput::LogAvailableController(int) const {}
void CControllerInput::ScanExistingControllers() {}
void CControllerInput::HandleDeviceAdded(int) {}
void CControllerInput::HandleDeviceRemoved(int) {}
void CControllerInput::HandleDeviceRemapped(int) {}
void CControllerInput::HandleButtonDown(int, int, std::uint8_t) {}
void CControllerInput::HandleButtonUp(int, int, std::uint8_t) {}
void CControllerInput::HandleAxisMotion(int, int, std::int16_t) {}

#endif
