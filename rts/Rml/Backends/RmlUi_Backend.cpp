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
#include <ranges>
#include <tracy/Tracy.hpp>

#include "System/Log/ILog.h"
#include "Lua/LuaUI.h"
#include "Rendering/GlobalRendering.h"
#include "Rml/Components/ElementLuaTexture.h"
#include "Rml/RmlInputReceiver.h"
#include "Rml/SolLua/RmlSolLua.h"
#include "RmlUi_Backend.h"
#include "Rml/SVG/SVGPlugin.h"

#ifndef HEADLESS
#include "RmlUi_Renderer_GL3_Recoil.h"
#else
#include "RmlUi_Renderer_Headless.h"
#endif

#include "RmlUi_SystemInterface.h"
#include "RmlUi_VFSFileInterface.h"
#include "System/Input/InputHandler.h"
#include "System/Log/ILog.h"

#define RML_DEBUG_HOST_CONTEXT_NAME "__debug_host_context__"

struct lua_State;

/**
  Global state used by this backend.
  Lifetime governed by the calls to Backend::Initialize() and
  Backend::Shutdown().

  Listens for Rml Create/Destroy Context events
 */
class BackendState : public Rml::Plugin {

public:
	int GetEventClasses() override
	{
		return EVT_BASIC;
	}

	void OnInitialise() override{};
	void OnShutdown() override{};

	void OnContextCreate(Rml::Context* context) override
	{
		RmlGui::OnContextCreate(context);
	};

	void OnContextDestroy(Rml::Context* context) override
	{
		RmlGui::OnContextDestroy(context);
	}

	RmlSystemInterface system_interface;
#ifndef HEADLESS
	RenderInterface_GL3_Recoil render_interface;
#else
	RenderInterface_Headless render_interface;
#endif
	VFSFileInterface file_interface;

	std::vector<Rml::Context*> contexts;
	std::unordered_set<Rml::Context*> contexts_to_remove;

	Rml::Context* debug_host_context = nullptr;
	Rml::Context* clicked_context = nullptr;

	InputHandler::HandlerTokenT inputCon;
	CRmlInputReceiver inputReceiver;

	bool initialized = false;
	int winX = 0;
	int winY = 0;

	lua_State* ls = nullptr;
	Rml::SolLua::SolLuaPlugin* luaPlugin = nullptr;

	RmlGui::SVG::DynamicSVGPlugin* svgPlugin;
	Rml::UniquePtr<Rml::ElementInstancerGeneric<RmlGui::ElementLuaTexture>> element_lua_texture_instancer;
};

static Rml::UniquePtr<BackendState> state;

bool RmlInitialized()
{
	return state && state->initialized;
}

bool RmlGui::Initialize()
{
	LOG_L(L_INFO, "[RmlUi::%s] Beginning RmlUi Initialization", __func__);
	state = Rml::MakeUnique<BackendState>();

	if (!((bool) state->render_interface)) {
		state.reset();
		LOG_L(L_ERROR, "[RmlGui::%s] Could not initialize render interface.", __func__);
		return false;
	}

	auto winX = globalRendering->winSizeX;
	auto winY = globalRendering->winSizeY;

	Rml::SetFileInterface(&state->file_interface);
	Rml::SetSystemInterface(RmlGui::GetSystemInterface());
	Rml::SetRenderInterface(RmlGui::GetRenderInterface());

	state->render_interface.SetViewport(winX, winY);
	state->winX = winX;
	state->winY = winY;

	Rml::Initialise();

	Rml::LoadFontFace("fonts/FreeSansBold.otf", true);
	state->inputCon = input.AddHandler(&RmlGui::ProcessEvent);
	state->initialized = true;

	state->element_lua_texture_instancer = Rml::MakeUnique<Rml::ElementInstancerGeneric<ElementLuaTexture>>();
	Rml::Factory::RegisterElementInstancer("texture", state->element_lua_texture_instancer.get());

	state->svgPlugin = RmlGui::SVG::Initialise();
	Rml::RegisterPlugin(state->svgPlugin);
	Rml::RegisterPlugin(state.get());

	return true;
}

bool RmlGui::InitializeLua(lua_State* lua_state)
{
	if (!RmlInitialized()) {
		RmlGui::Initialize();
	} else if (state->ls != nullptr) {
		return false;
	}

	LOG_L(L_INFO, "[RmlGui::%s] Initializing RmlUi Lua Bindings", __func__);

	sol::state_view lua(lua_state);
	state->ls = lua_state;
	state->luaPlugin = Rml::SolLua::Initialise(&lua, "rmlDocumentId");
	state->system_interface.SetTranslationTable(&state->luaPlugin->translationTable);
	return true;
}

bool RmlGui::RemoveLua()
{
	if (!RmlInitialized() || state->ls == nullptr) {
		return false;
	}

	// debugger must be shut down before the reference to
	// Lua Plugin DocumentElementInstancer becomes a dangling pointer
	if (state->debug_host_context) {
		MarkContextForRemoval(state->debug_host_context);
		state->debug_host_context = nullptr;
		Update();
		Rml::Debugger::Shutdown();
	}

	state->luaPlugin->RemoveLuaItems();

	// Update to allow clean up of removed items.
	Update();

	Rml::UnregisterPlugin(state->luaPlugin);
	state->system_interface.SetTranslationTable(nullptr);
	state->luaPlugin = nullptr;
	state->ls = nullptr;

	return true;
}

void RmlGui::Shutdown()
{
	if (!RmlInitialized()) {
		return;
	}

	if (state->debug_host_context) {
		Rml::Debugger::Shutdown();
	}

	// note: during SpringApp shutdown, RmlGui::RemoveLua() was already called when LuaUI was shutdown
	RemoveLua();
	Rml::UnregisterPlugin(state->svgPlugin);
	Rml::UnregisterPlugin(state.get());

	// removes all contexts, interfaces must be alive at this point
	Rml::Shutdown();

	// interfaces within can now be destroyed
	state.reset();
}

void RmlGui::Reload()
{
	if (RmlInitialized()) {
		LOG_L(L_NOTICE, "[RmlGui::%s] reloading: ", __func__);
		RmlGui::Shutdown();
		return;
	}
	RmlGui::Initialize();
}

void RmlGui::SetDebugContext(Rml::Context* context)
{
	if (!RmlInitialized()) {
		return;
	}

	if (state->debug_host_context == nullptr) {
		state->debug_host_context = Rml::CreateContext(RML_DEBUG_HOST_CONTEXT_NAME, {0, 0});

		// TODO?: Make own Debugger UI that better suits our needs
		Rml::Debugger::Initialise(state->debug_host_context);
	}

	Rml::Debugger::SetContext(context);
	Rml::Debugger::SetVisible(context != nullptr);
}

Rml::SystemInterface* RmlGui::GetSystemInterface()
{
	return &state->system_interface;
}

Rml::RenderInterface* RmlGui::GetRenderInterface()
{
	return &state->render_interface;
}

bool RmlGui::IsMouseInteractingWith()
{
	if (!RmlInitialized()) {
		return false;
	}
	return state->inputReceiver.IsAbove();
}

const std::string& RmlGui::GetMouseCursor()
{
	if (!RmlInitialized()) {
		static std::string empty = "";
		return empty;
	}
	return state->system_interface.GetMouseCursor();
}

void RmlGui::SetMouseCursorAlias(std::string from, std::string to) {
	if (!RmlInitialized()) {
		return;
	}
	state->system_interface.mouseCursorAliases.insert_or_assign(from, to);
}

CInputReceiver* RmlGui::GetInputReceiver()
{
	if (!RmlInitialized()) {
		return nullptr;
	}
	return &state->inputReceiver;
}

void RmlGui::OnContextCreate(Rml::Context* context)
{
	context->SetDimensions({state->winX, state->winY});
	if likely(state->debug_host_context || context->GetName() != RML_DEBUG_HOST_CONTEXT_NAME) {
		state->contexts.push_back(context);
	} else {
		state->contexts.insert(state->contexts.begin(), context);
	}
}

void RmlGui::OnContextDestroy(Rml::Context* context)
{
	state->contexts.erase(std::ranges::find(state->contexts, context));
}

Rml::Context* RmlGui::GetOrCreateContext(const std::string& name)
{
	if (!RmlInitialized()) {
		return nullptr;
	}

	Rml::Context* context = Rml::GetContext(name);
	if (context == nullptr) {
		context = Rml::CreateContext(name, {0, 0});
	} else {
		// can happen if name reused on the same frame
		state->contexts_to_remove.erase(context);
	}

	return context;
}

Rml::Context* RmlGui::GetContext(const std::string& name) {
	if (!RmlInitialized()) {
		return nullptr;
	}

	Rml::Context* context = Rml::GetContext(name);
	if (context != nullptr && !state->contexts_to_remove.contains(context)) {
		return context;
	}
	return nullptr;
}

void RmlGui::MarkContextForRemoval(Rml::Context *context) {
	if (!RmlInitialized() || context == nullptr) {
		return;
	}

	state->contexts_to_remove.insert(context);
}

void RmlGui::Update()
{
	ZoneScopedN("RmlGui Update");
	if (!RmlInitialized()) {
		return;
	}
#ifndef HEADLESS
	for (const auto& context : state->contexts) {
		context->Update();
	}

	// move clicked context to top
	if (state->clicked_context) {
		// debug context is always to be at index 0 so it renders on top
		auto start = state->contexts.begin() + (state->debug_host_context ? 1 : 0);
		auto context_pos = std::ranges::find(start, state->contexts.end(), state->clicked_context);
		if (context_pos != start && context_pos != state->contexts.end()) {
			std::ranges::rotate(start, context_pos, context_pos + 1);
		}
		state->clicked_context = nullptr;
	}

	if unlikely(!state->contexts_to_remove.empty()) {
		for (const auto& context : state->contexts_to_remove) {
			Rml::RemoveContext(context->GetName());
		}
		state->contexts_to_remove.clear();
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
	if (state->contexts.empty())
		return;

	RmlGui::BeginFrame();
	// render back-to-front so that index 0 is atop index 1 and so on
	for (auto& context: std::ranges::reverse_view(state->contexts)) {
		context->Render();
	}
	RmlGui::PresentFrame();
#endif
}

void RmlGui::BeginFrame()
{
	state->render_interface.BeginFrame();
}

void RmlGui::PresentFrame()
{
	state->render_interface.EndFrame();
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
	for (const auto& context : state->contexts) {
		result |= !RmlSDLRecoil::EventMouseMove(context, x, y);
		if (result) break;
	}
	state->inputReceiver.setActive(result);
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
	for (const auto& context : state->contexts) {
		bool handled = false;

		if (!result) {
			handled = !RmlSDLRecoil::EventMousePress(context, x, y, button);
			result |= handled;
		}

		if (!handled) {
			Rml::Element* el = context->GetFocusElement();
			if (el) {
				el->Blur();
			}
		} else if (state->debug_host_context && state->debug_host_context != context) {
			state->clicked_context = context;
		}
	}

	state->inputReceiver.setActive(result);
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
	for (const auto& context : state->contexts) {
		result |= !RmlSDLRecoil::EventMouseRelease(context, x, y, button);
		if (result) break;
	}
	state->inputReceiver.setActive(result);
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
	for (const auto& context : state->contexts) {
		result |= !RmlSDLRecoil::EventMouseWheel(context, delta);
		if (result) break;
	}
	state->inputReceiver.setActive(result);
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
	for (const auto& context : state->contexts) {
		auto kc = RmlSDLRecoil::ConvertKey(keyCode);
		result |= !RmlSDLRecoil::EventKeyDown(context, kc);
		if (result) break;
	}
	return result;
}

bool RmlGui::ProcessKeyReleased(int keyCode, int scanCode)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (const auto& context : state->contexts) {
		result |= !RmlSDLRecoil::EventKeyUp(context, RmlSDLRecoil::ConvertKey(keyCode));
		if (result) break;
	}
	return result;
}

bool RmlGui::ProcessTextInput(const std::string& text)
{
	if (!RmlInitialized()) {
		return false;
	}
	bool result = false;
	for (const auto& context : state->contexts) {
		result |= !RmlSDLRecoil::EventTextInput(context, text);
		if (result) break;
	}
	return result;
}

bool processContextEvent(Rml::Context* context, const SDL_Event& event)
{
	switch (event.type) {
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_TEXTINPUT:
			return true;  // handled elsewhere

		case SDL_WINDOWEVENT: {
			if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				auto x = event.window.data1;
				auto y = event.window.data2;

				state->render_interface.SetViewport(x, y);
				state->winX = x;
				state->winY = y;
			}
		} break;

		default:
			break;
	}

	return RmlSDLRecoil::InputEventHandler(context, event);
}

bool RmlGui::ProcessEvent(const SDL_Event& event)
{
	if (!RmlInitialized()) {
		return false;
	}

	bool result = false;
	for (const auto& context : state->contexts) {
		result |= !processContextEvent(context, event);
		if (result) break;
	}

	return result;
}

lua_State* RmlGui::GetLuaState()
{
    if (!RmlInitialized())
    {
        return nullptr;
    }

    return state->ls;
}
