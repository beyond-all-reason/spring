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

#include "RmlSolLua.h"

#include <sol2/sol.hpp>
#include <RmlUi/Core.h>

#include "bind/bind.h"


namespace Rml::SolLua
{

	SolLuaPlugin* Initialise(sol::state_view* state)
	{
		SolLuaPlugin* slp;
		if (state != nullptr)
		{
			slp = new SolLuaPlugin(*state);
			::Rml::RegisterPlugin(slp);
			RegisterLua(state, slp);
		}
		return slp;
	}

	SolLuaPlugin* Initialise(sol::state_view* state, const Rml::String& lua_environment_identifier)
	{
		SolLuaPlugin* slp;
		if (state != nullptr)
		{
			slp = new SolLuaPlugin(*state, lua_environment_identifier);
			::Rml::RegisterPlugin(slp);
			RegisterLua(state, slp);
		}
		return slp;
	}

	void RegisterLua(sol::state_view* state, SolLuaPlugin* slp)
	{
		sol::table namespace_table = state->create_named_table("RmlUi");

		bind_color(namespace_table);
		bind_context(namespace_table, slp);
		bind_datamodel(namespace_table);
		bind_element(namespace_table);
		bind_element_derived(namespace_table);
		bind_element_form(namespace_table);
		bind_document(namespace_table);
		bind_event(namespace_table);
		bind_global(namespace_table, slp);
		bind_vector(namespace_table);
		bind_convert(namespace_table);
	}

} // end namespace Rml::SolLua
