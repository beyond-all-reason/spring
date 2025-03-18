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

#pragma once

#include <RmlUi/Core/ElementDocument.h>

#include <sol2/sol.hpp>


namespace Rml::SolLua
{
	/// <summary>
	/// Lua error handler.
	/// </summary>
	/// <param name="">(Unused) Lua state.</param>
	/// <param name="pfr">The result that holds our error.</param>
	/// <returns>The error result.</returns>
	sol::protected_function_result ErrorHandler(lua_State*, sol::protected_function_result pfr);


	class SolLuaDocument : public ::Rml::ElementDocument
	{
	public:
		/// <summary>
		/// Construct the SolLuaDocument.
		/// </summary>
		/// <param name="state">The Lua state to register into.</param>
		/// <param name="tag">The document tag (body).</param>
		SolLuaDocument(sol::state_view state, const Rml::String& tag, const Rml::String& lua_env_identifier);

		/// <summary>
		/// Loads an inline script.
		/// </summary>
		/// <param name="context">The UI context.</param>
		/// <param name="source_path">Source path.</param>
		/// <param name="source_line">Source line.</param>
		void LoadInlineScript(const Rml::String& content, const Rml::String& source_path, int source_line) override;

		/// <summary>
		/// Loads a script from a file.
		/// </summary>
		/// <param name="source_path">The file to load.</param>
		void LoadExternalScript(const Rml::String& source_path) override;

		/// <summary>
		/// Runs a piece of Lua script within the environment with our error handler.
		/// </summary>
		/// <param name="script">The script to run.</param>
		/// <returns>The result of the script.</returns>
		sol::protected_function_result RunLuaScript(const Rml::String& script);

		/// <summary>
		/// Gets the Lua environment attached to this document.
		/// </summary>
		/// <returns>A reference to the Lua environment.</returns>
		sol::environment& GetLuaEnvironment() { return m_environment; }

		/// <summary>
		/// Gets the Lua environment identifier attached to this document.
		/// </summary>
		/// <returns>A const reference to the Lua environment identifier.</returns>
		const Rml::String& GetLuaEnvironmentIdentifier() const { return m_lua_env_identifier; }

		sol::environment m_environment;
	protected:
		sol::state_view m_state;
		Rml::String m_lua_env_identifier;
	};

} // namespace Rml::SolLua
