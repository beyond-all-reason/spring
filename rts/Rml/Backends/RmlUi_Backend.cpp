/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This source file is derived from the source code of RmlUi, the HTML/CSS
 * Interface Middleware
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

#include <RmlUi/Core.h>
#include <RmlUi/Core/Profiling.h>
#include <RmlUi/Debugger.h>
#include <SDL.h>
#include <functional>
#include <tracy/Tracy.hpp>

#include "Lua/LuaUI.h"
#include "Rendering/Textures/Bitmap.h"
#include "Rml/Components/ElementLuaTexture.h"
#include "Rml/RmlInputReceiver.h"
#include "Rml/SolLua/RmlSolLua.h"
#include "RmlUi_Backend.h"

#ifndef HEADLESS
#include "RmlUi_Renderer_GL3_Recoil.h"
#else
#include "RmlUi_Renderer_Headless.h"
#endif

#include "RmlUi_SystemInterface.h"
#include "RmlUi_VFSFileInterface.h"
#include "System/Input/InputHandler.h"
#include "System/Log/ILog.h"

using CtxMutex = std::recursive_mutex;
using CtxLockGuard = std::lock_guard<CtxMutex>;
struct lua_State;

/// Passes through RML events to the function pointers given in the constructor
class PassThroughPlugin : public Rml::Plugin
{
	void (*onContextCreate)(Rml::Context*);
	void (*onContextDestroy)(Rml::Context*);

public:
	PassThroughPlugin(void (*onContextCreate)(Rml::Context*),
	                  void (*onContextDestroy)(Rml::Context*))
		: onContextCreate{onContextCreate}
		, onContextDestroy{onContextDestroy}
	{
	}

	int GetEventClasses() override
	{
		return EVT_BASIC;
	}

	void OnInitialise() override{};
	void OnShutdown() override{};

	void OnContextCreate(Rml::Context* context) override
	{
		onContextCreate(context);
	};

	void OnContextDestroy(Rml::Context* context) override
	{
		onContextDestroy(context);
	};
};

/**
  Global data used by this backend.
  Lifetime governed by the calls to Backend::Initialize() and
  Backend::Shutdown().
 */
struct BackendData {
	RmlSystemInterface system_interface;
#ifndef HEADLESS
	RenderInterface_GL3_Recoil render_interface;
#else
	RenderInterface_Headless render_interface;
#endif
	VFSFileInterface file_interface;

	SDL_Window* window = nullptr;
	SDL_GLContext gl_context = nullptr;
	std::vector<Rml::Context*> contexts;
	InputHandler::SignalType::connection_type inputCon;
	CRmlInputReceiver inputReceiver;

	// make atomic_bool?
	bool initialized = false;
	bool debuggerAttached = false;
	int winX = 1;
	int winY = 1;

	lua_State* ls = nullptr;
	Rml::SolLua::SolLuaPlugin* luaPlugin = nullptr;

	CtxMutex contextMutex;
	Rml::UniquePtr<PassThroughPlugin> plugin;
    Rml::UniquePtr<Rml::ElementInstancerGeneric<RmlGui::ElementLuaTexture>> element_lua_texture_instancer;
};

static Rml::UniquePtr<BackendData> data;

bool RmlInitialized()
{
	return data && data->initialized;
}

bool RmlGui::Initialize(SDL_Window* target_window, SDL_GLContext target_glcontext, int winX,
                        int winY)
{
	data = Rml::MakeUnique<BackendData>();

	if (!data->render_interface) {
		data.reset();
		fprintf(stderr, "Could not initialize OpenGL3 render interface.");
		return false;
	}

	data->window = target_window;
	data->gl_context = target_glcontext;

	Rml::SetFileInterface(&data->file_interface);
	Rml::SetSystemInterface(RmlGui::GetSystemInterface());
	Rml::SetRenderInterface(RmlGui::GetRenderInterface());

	data->render_interface.SetViewport(winX, winY);
	data->winX = winX;
	data->winY = winY;

	Rml::Initialise();

	Rml::LoadFontFace("fonts/FreeSansBold.otf", true);
	data->inputCon = input.AddHandler(&RmlGui::ProcessEvent);
	data->initialized = true;

	data->element_lua_texture_instancer = Rml::MakeUnique<Rml::ElementInstancerGeneric<ElementLuaTexture>>();
	Rml::Factory::RegisterElementInstancer("texture", data->element_lua_texture_instancer.get());

	data->plugin = Rml::MakeUnique<PassThroughPlugin>(OnContextCreate, OnContextDestroy);
	Rml::RegisterPlugin(data->plugin.get());

	return true;
}

bool RmlGui::InitializeLua(lua_State* lua_state)
{
	if (!RmlInitialized()) {
		return false;
	}
	sol::state_view lua(lua_state);
	data->ls = lua_state;
	data->luaPlugin = Rml::SolLua::Initialise(&lua, "rmlDocumentId");
	data->system_interface.SetTranslationTable(&data->luaPlugin->translationTable);
	return true;
}

bool RmlGui::RemoveLua()
{
	if (!RmlInitialized() || !data->ls) {
		return false;
	}
	data->luaPlugin->RemoveLuaItems();
	Update();
	Rml::UnregisterPlugin(data->luaPlugin);
	data->system_interface.SetTranslationTable(nullptr);
	data->luaPlugin = nullptr;
	data->ls = nullptr;

	return true;
}

void RmlGui::Shutdown()
{
	if (!RmlInitialized()) {
		return;
	}

	Rml::UnregisterPlugin(data->plugin.get());
	data.reset();

	// removes all contexts
	Rml::Shutdown();
}

void RmlGui::Reload()
{
	if (!RmlInitialized()) {
		return;
	}
	LOG_L(L_NOTICE, "[RmlGui::%s] reloading: ", __func__);
	SDL_Window* window = data->window;
	SDL_GLContext gl_context = data->gl_context;
	int winX = data->winX;
	int winY = data->winY;
	RmlGui::Shutdown();
	RmlGui::Initialize(window, gl_context, winX, winY);
}

void RmlGui::ToggleDebugger(int contextIndex)
{
	if (data->debuggerAttached) {
		// TODO: Ensure thread safety somehow if needed
		Rml::Debugger::Initialise(data->contexts[contextIndex]);
		Rml::Debugger::SetVisible(true);
	} else {
		Rml::Debugger::Shutdown();
	}
	data->debuggerAttached = !data->debuggerAttached;
}

Rml::SystemInterface* RmlGui::GetSystemInterface()
{
	return &data->system_interface;
}

Rml::RenderInterface* RmlGui::GetRenderInterface()
{
	return &data->render_interface;
}

bool RmlGui::IsMouseInteractingWith()
{
	if (!RmlInitialized()) {
		return false;
	}
	return data->inputReceiver.IsAbove();
}

const std::string& RmlGui::GetMouseCursor()
{
	if (!RmlInitialized()) {
		static std::string empty = "";
		return empty;
	}
	return data->system_interface.GetMouseCursor();
}

void RmlGui::SetMouseCursorAlias(std::string from, std::string to) {
	if (!RmlInitialized()) {
		return;
	}
	data->system_interface.mouseCursorAliases.insert_or_assign(from, to);
}

CInputReceiver* RmlGui::GetInputReceiver()
{
	if (!RmlInitialized()) {
		return nullptr;
	}
	return &data->inputReceiver;
}

void RmlGui::OnContextCreate(Rml::Context* context)
{
	CtxLockGuard lock(data->contextMutex);
	context->SetDimensions({data->winX, data->winY});
	data->contexts.push_back(context);
}

void RmlGui::OnContextDestroy(Rml::Context* context)
{
	CtxLockGuard lock(data->contextMutex);
	data->contexts.erase(std::ranges::find(data->contexts, context));
}

void RmlGui::Update()
{
	ZoneScopedN("RmlGui Update");
	if (!RmlInitialized()) {
		return;
	}
#ifndef HEADLESS
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		context->Update();
	}
#endif
}

void RmlGui::RenderFrame()
{
	ZoneScopedN("RmlGui Draw");
	if (!RmlInitialized()) {
		return;
	}

#ifndef HEADLESS
	RmlGui::BeginFrame();
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		context->Render();
	}
	RmlGui::PresentFrame();
#endif
}

void RmlGui::BeginFrame()
{
	// data->render_interface.Clear();
	data->render_interface.BeginFrame();
}

void RmlGui::PresentFrame()
{
	data->render_interface.EndFrame();
	RMLUI_FrameMark;
}

/*
  Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessMouseMove(int x, int y, int dx, int dy, int button)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		result |= !RmlSDLRecoil::EventMouseMove(context, x, y);
	}
	data->inputReceiver.setActive(result);
	return result;
}

/*
  Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessMousePress(int x, int y, int button)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		bool handled = !RmlSDLRecoil::EventMousePress(context, x, y, button);
		result |= handled;
		if (!handled) {
			Rml::Element* el = context->GetFocusElement();
			if (el) {
				el->Blur();
			}
		}
	}
	data->inputReceiver.setActive(result);
	return result;
}

/*
  Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessMouseRelease(int x, int y, int button)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		result |= !RmlSDLRecoil::EventMouseRelease(context, x, y, button);
	}
	data->inputReceiver.setActive(result);
	return result;
}

/*
  Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessMouseWheel(float delta)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		result |= !RmlSDLRecoil::EventMouseWheel(context, delta);
	}
	data->inputReceiver.setActive(result);
	return result;
}

/*
  Return true if the event was handled by rmlui
*/
bool RmlGui::ProcessKeyPressed(int keyCode, int scanCode, bool isRepeat)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		auto kc = RmlSDLRecoil::ConvertKey(keyCode);
		result |= !RmlSDLRecoil::EventKeyDown(context, kc);
	}
	return result;
}

bool RmlGui::ProcessKeyReleased(int keyCode, int scanCode)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		result |= !RmlSDLRecoil::EventKeyUp(context, RmlSDLRecoil::ConvertKey(keyCode));
	}
	return result;
}

bool RmlGui::ProcessTextInput(const std::string& text)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		result |= !RmlSDLRecoil::EventTextInput(context, text);
	}
	return result;
}

bool processContextEvent(Rml::Context* context, const SDL_Event& event)
{
	switch (event.type) {
		case SDL_WINDOWEVENT: {
			switch (event.window.event) {
				case SDL_WINDOWEVENT_SIZE_CHANGED: {
					Rml::Vector2i dimensions(event.window.data1, event.window.data2);
					data->render_interface.SetViewport(dimensions.x, dimensions.y);
					data->winX = dimensions.x;
					data->winY = dimensions.y;
				} break;
			}
			RmlSDLRecoil::InputEventHandler(context, event);
		} break;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_TEXTINPUT:
			break;  // handled elsewhere
		default: {
			RmlSDLRecoil::InputEventHandler(context, event);
		} break;
	}
	// these events are not captured, and should continue propagating
	return false;
}

bool RmlGui::ProcessEvent(const SDL_Event& event)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (CtxLockGuard lock(data->contextMutex); const auto& context : data->contexts) {
		result |= processContextEvent(context, event);
	}
	return result;
}

lua_State* RmlGui::GetLuaState()
{
    if (!RmlInitialized())
    {
        return nullptr;
    }

    return data->ls;
}
