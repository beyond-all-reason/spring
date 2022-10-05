/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CONTROLLER_INPUT_H
#define CONTROLLER_INPUT_H

#include <SDL_gamecontroller.h>
#include <slimsig/connection.h>
#include "System/Input/InputHandler.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"

class ControllerInput
{
public:
	ControllerInput();
	~ControllerInput();

	static void InitStatic();
	static void KillStatic();

	bool HandleSDLControllerEvent(const SDL_Event& event);

	SDL_GameController* ConnectController(int deviceId, int& instanceId);
	SDL_GameController* GetConnectedController(int instanceId);
	bool DisconnectController(int instanceId);
	bool DisconnectController(int instanceId, SDL_GameController* controller);
	void DisconnectControllers();

protected:
	typedef spring::unsynced_map<int, SDL_GameController*> ControllerMap;
	typedef spring::unsynced_set<int> ControllerSet;

protected:
	ControllerMap connectedControllers;

public:
	ControllerSet GetConnectedControllers();

protected:
	SDL_GameController* GetController(int instanceId);

protected:
	InputHandler::SignalType::connection_type inputCon;
};

extern ControllerInput* controllerInput;

#endif /* CONTROLLER_INPUT_H */
