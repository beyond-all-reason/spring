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

#include "bind.h"

#include "../plugin/SolLuaDataModel.h"


namespace Rml::SolLua
{
	namespace functions
	{
		sol::object dataModelGet(SolLuaDataModel& self, const std::string& name, sol::this_state s)
		{
			return self.Table.get<sol::object>(name);
		}

		void dataModelSet(SolLuaDataModel& self, const std::string& name, sol::object value, sol::this_state s)
		{
			self.Table.set(name, value);
			self.Handle.DirtyVariable(name);
		}

		void dataModelSetDirty(SolLuaDataModel& self, const std::string& name)
		{
			self.Handle.DirtyVariable(name);
		}
	}

	void bind_datamodel(sol::table& namespace_table)
	{
		namespace_table.new_usertype<SolLuaDataModel>("SolLuaDataModel", sol::no_constructor,
			sol::meta_function::index, &functions::dataModelGet,
			sol::meta_function::new_index, &functions::dataModelSet,
			"__SetDirty", &functions::dataModelSetDirty
		);
	}

} // end namespace Rml::SolLua
