/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This source file is derived from the code
 * at https://github.com/LoneBoco/RmlSolLua
 * which is under the following license:
 *
 * MIT License
 *
 * Copyright (c) 2022 John Norman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SolLuaPlugin.h"

#include "RmlUi/Core/Context.h"
#include "SolLuaInstancer.h"
#include "Rml/Backends/RmlUi_Backend.h"
#include <RmlUi/Core.h>

#include <algorithm>


namespace Rml::SolLua
{
	SolLuaPlugin::SolLuaPlugin(sol::state_view lua_state)
		: m_lua_state{lua_state}
	{
	}

	SolLuaPlugin::SolLuaPlugin(sol::state_view lua_state, const Rml::String& lua_environment_identifier)
		: m_lua_state{lua_state}, m_lua_env_identifier{lua_environment_identifier}
	{
	}

	int SolLuaPlugin::GetEventClasses()
	{
		return EVT_BASIC | EVT_DOCUMENT;
	}

	void SolLuaPlugin::AddContextTracking(Context* context) {
		luaContexts.emplace_back(context);
	}

	void SolLuaPlugin::OnContextDestroy(Context* context) {
		luaContexts.erase(std::remove(luaContexts.begin(), luaContexts.end(), context), luaContexts.end());
	}

	void SolLuaPlugin::AddDocumentTracking(ElementDocument* document) {
		luaDocuments.emplace_back(document);
	}

	void SolLuaPlugin::OnDocumentUnload(ElementDocument* document) {
		luaDocuments.erase(std::remove(luaDocuments.begin(), luaDocuments.end(), document), luaDocuments.end());
	}

	void SolLuaPlugin::RemoveLuaItems(){
		for(auto d: luaDocuments) {
			d->Close();
		}
		for(auto c: luaContexts) {
			RmlGui::MarkContextForRemoval(c);
		}
	}

	void SolLuaPlugin::OnInitialise()
	{
		document_element_instancer = std::make_unique<SolLuaDocumentElementInstancer>(m_lua_state, m_lua_env_identifier);
		event_listener_instancer = std::make_unique<SolLuaEventListenerInstancer>(m_lua_state);
		Factory::RegisterElementInstancer("body", document_element_instancer.get());
		Factory::RegisterEventListenerInstancer(event_listener_instancer.get());
	}

	void SolLuaPlugin::OnShutdown()
	{
		// m_lua_state.collect_garbage();
		delete this;
	}
} // end namespace Rml::SolLua
