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

#include "plugin/SolLuaPlugin.h"
#include <RmlUi/Core.h>

namespace sol
{
	class state_view;
}

namespace Rml::SolLua
{
	/// <summary>
	/// Initializes RmlSolLua using the supplied Lua state.
	/// </summary>
	/// <param name="state">The Lua state to initialize into.</param>
	SolLuaPlugin* Initialise(sol::state_view* state);

	/// <summary>
	/// Initializes RmlSolLua using the supplied Lua state.
	/// Sets the Lua variable specified by lua_environment_identifier to the document's id when running Lua code.
	/// </summary>
	/// <param name="state">The Lua state to initialize into.</param>
	/// <param name="lua_environment_identifier">The Lua variable name that is set to the document's id.</param>
	SolLuaPlugin* Initialise(sol::state_view* state, const Rml::String& lua_environment_identifier);

	/// <summary>
	/// Registers RmlSolLua into the specified Lua state.
	/// </summary>
	/// <param name="state">The Lua state to register into.</param>
	void RegisterLua(sol::state_view* state, SolLuaPlugin* slp);

} // end namespace Rml::SolLua
