/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
	This workaround fixes the windows slow mouse movement problem
	(happens on full-screen mode + pressing keys).
	The code hacks around the mouse input from DirectInput,
	which SDL uses in full-screen mode.
	Instead it installs a window message proc and reads input from WM_MOUSEMOVE.
	On non-windows, the normal SDL events are used for mouse input

	new:
	It also workarounds a issue with SDL+windows and hardware cursors
	(->it has to block WM_SETCURSOR),
	so it is used now always even in window mode!

	newer:
	SDL_Event struct is used for new input handling.
	Several people confirmed its working.
*/


#include "MouseInput.h"
#include "InputHandler.h"

#include "Game/UI/MouseHandler.h"
#include "Rendering/GlobalRendering.h"
#include "System/MainDefines.h"
#include "System/SafeUtil.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>


IMouseInput* mouseInput = nullptr;

IMouseInput::IMouseInput(bool relModeWarp)
{
	inputCon = input.AddHandler([this](const SDL_Event& event) { return this->HandleSDLMouseEvent(event); });
	// NOTE: SDL_HINT_MOUSE_RELATIVE_MODE_WARP was removed in SDL3.
	// Relative mouse mode is now controlled via SDL_SetWindowRelativeMouseMode().
}

IMouseInput::~IMouseInput()
{
}


	bool IMouseInput::HandleSDLMouseEvent(const SDL_Event& event)
	{
		switch (event.type) {
			case SDL_EVENT_MOUSE_MOTION: {
				mousepos = int2(event.motion.x, event.motion.y);

				if (mouse != nullptr)
					mouse->MouseMove(mousepos.x, mousepos.y, event.motion.xrel, event.motion.yrel);

			} break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN: {
				mousepos = int2(event.button.x, event.button.y);

				// suppress if the button is already held via input emulation
				if (mouse != nullptr && !mouse->IsButtonEmulated(event.button.button))
					mouse->MousePress(mousepos.x, mousepos.y, event.button.button);

			} break;
			case SDL_EVENT_MOUSE_BUTTON_UP: {
				mousepos = int2(event.button.x, event.button.y);

				if (mouse != nullptr && !mouse->IsButtonEmulated(event.button.button))
					mouse->MouseRelease(mousepos.x, mousepos.y, event.button.button);

			} break;
			case SDL_EVENT_MOUSE_WHEEL: {
				if (mouse != nullptr)
					mouse->MouseWheel(event.wheel.y);

			} break;
			case SDL_EVENT_WINDOW_MOUSE_ENTER: {
				if (mouse != nullptr)
					mouse->WindowEnter();
			} break;
			case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
			// mouse left window; set pos internally to view center-pixel to prevent endless scrolling
			mousepos = {
				globalRendering->viewPosX          + (globalRendering->viewSizeX >> 1),
				globalRendering->viewWindowOffsetY + (globalRendering->viewSizeY >> 1)
			};

			if (mouse != nullptr)
				mouse->WindowLeave();
		} break;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////

#if defined(_WIN32) && !defined(HEADLESS)

#include <windows.h>

class CWin32MouseInput : public IMouseInput
{
public:
	static CWin32MouseInput* inst;

	LONG_PTR sdl_wndproc;
	HWND wnd;
	HCURSOR hCursor;

	static LRESULT CALLBACK SpringWndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg) {
			case WM_SETCURSOR: {
				if (inst->hCursor != nullptr) {
					const Uint16 hittest = LOWORD(lParam);

					if (hittest == HTCLIENT) {
						SetCursor(inst->hCursor);
						return TRUE;
					}
				}
			} break;
		}
		return CallWindowProc((WNDPROC)inst->sdl_wndproc, wnd, msg, wParam, lParam);
	}

	void SetWMMouseCursor(void* wmcursor)
	{
		hCursor = (HCURSOR)wmcursor;
	}

	void InstallWndCallback()
	{
		const SDL_PropertiesID props = SDL_GetWindowProperties(globalRendering->GetWindow());
		wnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
		if (!wnd)
			return;

		LONG_PTR cur_wndproc = GetWindowLongPtr(wnd, GWLP_WNDPROC);

		if (cur_wndproc != (LONG_PTR)SpringWndProc) {
			sdl_wndproc = GetWindowLongPtr(wnd, GWLP_WNDPROC);
			SetWindowLongPtr(wnd, GWLP_WNDPROC, (LONG_PTR)SpringWndProc);
		}
	}

	CWin32MouseInput(bool relModeWarp): IMouseInput(relModeWarp)
	{
		inst = this;
		hCursor = nullptr;
		sdl_wndproc = 0;
		wnd = 0;

		InstallWndCallback();
	}
	~CWin32MouseInput()
	{
		// reinstall the SDL window proc
		SetWindowLongPtr(wnd, GWLP_WNDPROC, sdl_wndproc);
	}
};

CWin32MouseInput* CWin32MouseInput::inst = nullptr;


alignas(CWin32MouseInput) static std::byte mouseInputMem[sizeof(CWin32MouseInput)];
#else
alignas(IMouseInput) static std::byte mouseInputMem[sizeof(IMouseInput)];
#endif



#if 1
static SDL_Event events[100];
#endif

bool IMouseInput::SetPos(int2 pos)
{
	if (!globalRendering->active)
		return false;

	// calling SDL_WarpMouse at 300fps eats ~5% cpu usage, so only update when needed
	if (pos.x == mousepos.x && pos.y == mousepos.y)
		return false;

	return (mousepos = pos, true);
}

bool IMouseInput::WarpPos(int2 pos)
{
	#if __unix__
		/* Needed for SDL3+Wayland where warping isn't allowed otherwise, works fine with X11.
		 * One would think there should be a corresponding `SDL_ShowCursor();` below,
		 * but apparently this prevents this work-around from working (?!). */
		SDL_HideCursor();
	#endif

	SDL_WarpMouseInWindow(globalRendering->GetWindow(), pos.x, pos.y);

	// SDL_WarpMouse generates SDL_EVENT_MOUSE_MOTION events
	// in `middle click scrolling` those SDL generated ones would point into
	// the opposite direction the user moved the mouse, and so events would
	// cancel each other -> camera wouldn't move at all or jitter
	// need to catch the SDL generated events and delete them from its queue
	//
	// NOTE [2018]:
	//   the above comment dates back to 2010, but also describes the recent
	//   Windows 10 FCU bug with relative mode warping which similarly relies
	//   on WMIW
	#if 1
	// SDL_PeepEvents with SDL_GETEVENT auto-pumps events in SDL3
	SDL_PeepEvents(&events[0], sizeof(events) / sizeof(events[0]), SDL_GETEVENT, SDL_EVENT_MOUSE_MOTION, SDL_EVENT_MOUSE_MOTION);
	#else
	// should be equivalent, but for some reason is not
	SDL_FlushEvents(SDL_EVENT_MOUSE_MOTION);
	#endif

	return true;
}



IMouseInput* IMouseInput::GetInstance(bool relModeWarp)
{
	if (mouseInput == nullptr) {
#if defined(_WIN32) && !defined(HEADLESS)
		mouseInput = new (mouseInputMem) CWin32MouseInput(relModeWarp);
#else
		mouseInput = new (mouseInputMem) IMouseInput(relModeWarp);
#endif
	}

	return mouseInput;
}

void IMouseInput::FreeInstance(IMouseInput* mouseInp) {
	assert(mouseInp == mouseInput);
	spring::SafeDestruct(mouseInp);
	memset(mouseInputMem, 0, sizeof(mouseInputMem));
	mouseInput = nullptr;
}

