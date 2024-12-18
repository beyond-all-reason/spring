/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This source file is derived from the source code of RmlUi, the HTML/CSS Interface Middleware
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

#include "Rml/Rml_MathTypes_Conversions.h"
#include <RmlUi/Core.h>
#include <SDL.h>

#include "Game/UI/InputReceiver.h"
#include "lib/sol2/sol.hpp"

namespace RmlGui
{
	bool Initialize();
	bool InitializeLua(lua_State* lua_state);
	bool RemoveLua();

	void Shutdown();
	void Reload();

	Rml::SystemInterface* GetSystemInterface();
	Rml::RenderInterface* GetRenderInterface();

	bool ProcessEvent(const SDL_Event& event);

	bool ProcessKeyPressed(int keyCode, int scanCode, bool isRepeat);
	bool ProcessKeyReleased(int keyCode, int scanCode);
	bool ProcessTextInput(const std::string& text);
	bool ProcessMouseMove(int x, int y, int dx, int dy, int button);
	bool ProcessMousePress(int x, int y, int button);
	bool ProcessMouseRelease(int x, int y, int button);
	bool ProcessMouseWheel(float delta);

	void SetDebugContext(Rml::Context* context);

	bool IsMouseInteractingWith();
	const std::string& GetMouseCursor();
	void SetMouseCursorAlias(std::string from, std::string to);
	CInputReceiver* GetInputReceiver();
	lua_State* GetLuaState();

	void Update();
	void RenderFrame();

	void OnContextCreate(Rml::Context* context);
	void OnContextDestroy(Rml::Context* context);
	
	Rml::Context* GetOrCreateContext(const std::string& name);
	Rml::Context* GetContext(const std::string& name);
	void MarkContextForRemoval(Rml::Context* context);

	void BeginFrame();
	void PresentFrame();

}  // namespace RmlGui

#endif
