/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019-2023 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef RMLUI_BACKENDS_BACKEND_H
#define RMLUI_BACKENDS_BACKEND_H

#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/Types.h>
#include <SDL.h>
#include "lib/sol2/sol.hpp"
#include "Rml/Backends/RmlUi_Platform_SDL.h"

using KeyDownCallback = bool (*)(Rml::Context *context, Rml::Input::KeyIdentifier key, int key_modifier, float native_dp_ratio, bool priority);

namespace RmlGui
{

	bool Initialize(SDL_Window *target_window, SDL_GLContext target_glcontext, int winX, int winY);
	bool InitializeLua(lua_State *lua_state);

	void Shutdown();
	void Reload();

	Rml::SystemInterface *GetSystemInterface();
	Rml::RenderInterface *GetRenderInterface();

	bool ProcessEvent(const SDL_Event &event);

	bool ProcessKeyPressed(int keyCode, int scanCode, bool isRepeat);
	bool ProcessKeyReleased(int keyCode, int scanCode);
	bool ProcessTextInput(const std::string &text);
	bool ProcessMouseMove(int x, int y, int dx, int dy, int button);
	bool ProcessMousePress(int x, int y, int button);
	bool ProcessMouseRelease(int x, int y, int button);
	bool ProcessMouseWheel(float delta);

	void ToggleDebugger(int contextIndex);
	bool IsActive();

	void Update();
	void RenderFrame();

	void CreateContext(const std::string &name);
	void AddContext(Rml::Context *context);

	void BeginFrame();
	void PresentFrame();

}

#endif
