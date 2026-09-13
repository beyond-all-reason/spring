/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <SDL_events.h>
#include <SDL_gamecontroller.h>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "System/Input/InputHandler.h"

class CControllerInput
{
public:
	static CControllerInput* GetInstance();
	static void FreeInstance();

	CControllerInput();
	~CControllerInput();

	struct ControllerState {
		int deviceID = -1;
		int instanceID = -1;
		std::string name;

		std::array<std::int16_t, SDL_CONTROLLER_AXIS_MAX> axes = {};
		std::array<std::uint8_t, SDL_CONTROLLER_BUTTON_MAX> buttons = {};
	};

	std::vector<ControllerState> GetAvailableControllers() const;
	std::optional<ControllerState> GetControllerState(int instanceID) const;

	bool HandleSDLControllerEvent(const SDL_Event& event);

private:
	struct TrackedControllerState : public ControllerState {
		SDL_GameController* gameController = nullptr;
	};

	void LogAvailableController(int deviceID) const;
	void ScanExistingControllers();
	void HandleDeviceAdded(int deviceID);
	void HandleDeviceRemoved(int instanceID);
	void HandleDeviceRemapped(int instanceID);
	void HandleButtonDown(int instanceID, int buttonID, std::uint8_t value);
	void HandleButtonUp(int instanceID, int buttonID, std::uint8_t value);
	void HandleAxisMotion(int instanceID, int axisID, std::int16_t value);

private:
	InputHandler::HandlerTokenT inputCon;
	std::unordered_map<int, TrackedControllerState> controllersByInstanceID;
};

extern CControllerInput* controllerInput;
