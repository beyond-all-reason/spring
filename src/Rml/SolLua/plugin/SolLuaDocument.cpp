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

#include "SolLuaDocument.h"

#include <RmlUi/Core/Stream.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Context.h>


namespace Rml::SolLua
{

	sol::protected_function_result ErrorHandler(lua_State* l, sol::protected_function_result pfr)
	{
		if (!pfr.valid())
		{
			sol::error err = pfr;
			Rml::Log::Message(Rml::Log::LT_ERROR, "[Lua] %s", err.what());
			// sol::state_view lua(l);
			// auto handle = lua["__HandleError"];
			// sol::protected_function pf = handle;
			// pf();
		}
		return pfr;
	}

	//-----------------------------------------------------

	SolLuaDocument::SolLuaDocument(sol::state_view state, const Rml::String& tag, const Rml::String& lua_env_identifier)
		: m_state(state), ElementDocument(tag), m_environment(state, sol::create, state.globals()), m_lua_env_identifier(lua_env_identifier)
	{
	}

	void SolLuaDocument::LoadInlineScript(const Rml::String& content, const Rml::String& source_path, int source_line)
	{
		auto* context = GetContext();

		Rml::String buffer{ "--" };
		buffer.append("[");
		buffer.append(context->GetName());
		buffer.append("][");
		buffer.append(GetSourceURL());
		buffer.append("]:");
		buffer.append(Rml::ToString(source_line));
		buffer.append("\n");
		buffer.append(content);

		if (!m_lua_env_identifier.empty())
			m_environment[m_lua_env_identifier] = GetId();

		m_state.safe_script(buffer, m_environment, ErrorHandler);
	}

	void SolLuaDocument::LoadExternalScript(const String& source_path)
	{
		if (!m_lua_env_identifier.empty())
			m_environment[m_lua_env_identifier] = GetId();

		m_state.safe_script_file(source_path, m_environment, ErrorHandler);
	}

	sol::protected_function_result SolLuaDocument::RunLuaScript(const Rml::String& script)
	{
		if (!m_lua_env_identifier.empty())
			m_environment[m_lua_env_identifier] = GetId();

		return m_state.safe_script(script, m_environment, ErrorHandler);
	}

} // namespace Rml::SolLua
