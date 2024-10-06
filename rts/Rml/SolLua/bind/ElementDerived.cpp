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

#include "../plugin/SolLuaEventListener.h"

#include <unordered_map>


namespace Rml::SolLua
{

	void bind_element_derived(sol::table& namespace_table)
	{

		namespace_table.new_usertype<Rml::ElementText>("ElementText", sol::no_constructor,
			// G
			"text", sol::property(&Rml::ElementText::GetText, &Rml::ElementText::SetText),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);

		///////////////////////////

		namespace_table.new_usertype<Rml::ElementTabSet>("ElementTabSet", sol::no_constructor,
			// M
			"SetPanel", sol::resolve<void(int, const Rml::String&)>(&Rml::ElementTabSet::SetPanel),
			"SetTab", sol::resolve<void(int, const Rml::String&)>(&Rml::ElementTabSet::SetTab),
			//--
			"RemoveTab", &Rml::ElementTabSet::RemoveTab,

			// G+S
			"active_tab", sol::property(&Rml::ElementTabSet::GetActiveTab, &Rml::ElementTabSet::SetActiveTab),

			// G
			"num_tabs", &Rml::ElementTabSet::GetNumTabs,

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);

		///////////////////////////

		//--
		namespace_table.new_usertype<Rml::ElementProgress>("ElementProgress", sol::no_constructor,
			// G+S
			//--
			"value", sol::property(&Rml::ElementProgress::GetValue, &Rml::ElementProgress::SetValue),
			"max", sol::property(&Rml::ElementProgress::GetMax, &Rml::ElementProgress::SetMax),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);
	}

} // end namespace Rml::SolLua
