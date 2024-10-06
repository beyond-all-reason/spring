/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <vector>
#include <functional>
#include <SDL_events.h>

/**
 * @brief Simple thing: events go in, events come out
 *
 */
class InputHandler
{
public:
	class HandlerTokenT {
	public:
		friend class InputHandler;
		constexpr HandlerTokenT()
			: ih(nullptr)
			, pos(0)
		{}
		HandlerTokenT(InputHandler& ih_, size_t pos_)
			: ih(&ih_)
			, pos(pos_)
		{}
		HandlerTokenT(const HandlerTokenT&) = delete;
		HandlerTokenT(HandlerTokenT&& other) noexcept { *this = std::move(other); }
		~HandlerTokenT()
		{
			if (ih) {
				ih->eventHandlers[pos] = nullptr;
			}
		}

		HandlerTokenT& operator=(const HandlerTokenT&) = delete;
		HandlerTokenT& operator=(HandlerTokenT&& other) noexcept {
			std::swap(ih, other.ih);
			std::swap(pos, other.pos);

			return *this;
		}
	private:
		InputHandler* ih;
		size_t pos;
	};
public:
	using HandlerFuncT = std::function<bool(const SDL_Event&)>;

	InputHandler();

	void PushEvent(const SDL_Event& ev);
	void PushEvents();

	HandlerTokenT AddHandler(HandlerFuncT func);
private:
	std::vector<HandlerFuncT> eventHandlers;
};

extern InputHandler input;

#endif // INPUT_HANDLER_H
