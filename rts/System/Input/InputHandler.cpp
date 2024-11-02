/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InputHandler.h"
#include "System/TimeProfiler.h"

InputHandler input;

InputHandler::InputHandler() = default;

void InputHandler::PushEvent(const SDL_Event& ev)
{
	for (const auto& eventHandler : eventHandlers) {
		if (eventHandler) {
			if (eventHandler(ev))
				break;
		}
	}
}

void InputHandler::PushEvents()
{
	SCOPED_TIMER("Misc::InputHandler::PushEvents");

	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		// SDL_PollEvent may modify FPU flags
		streflop::streflop_init<streflop::Simple>();
		PushEvent(event);
	}
}

InputHandler::HandlerTokenT InputHandler::AddHandler(InputHandler::HandlerFuncT func)
{
	for (size_t i = 0; i < eventHandlers.size(); ++i) {
		if (eventHandlers[i] == nullptr) {
			eventHandlers[i] = func;
			return InputHandler::HandlerTokenT{ *this, i};
		}
	}
	eventHandlers.emplace_back(func);
	return InputHandler::HandlerTokenT{ *this, eventHandlers.size() - 1 };
}


