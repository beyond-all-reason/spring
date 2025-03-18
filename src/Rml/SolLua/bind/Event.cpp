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


namespace Rml::SolLua
{

	namespace functions
	{
		SolObjectMap getParameters(Rml::Event& self, sol::this_state s)
		{
			SolObjectMap result;

			const auto& parameters = self.GetParameters();
			for (auto& [key, value] : parameters)
			{
				if (self.GetId() == Rml::EventId::Tabchange && value.GetType() == Rml::Variant::INT)
				{
					auto object = sol::make_object(s, value.Get<int>());
					result.insert(std::make_pair(key, object));
				}
				else
				{
					auto object = makeObjectFromVariant(&value, s);
					result.insert(std::make_pair(key, object));
				}
			}

			return result;
		}
	}

	void bind_event(sol::table& namespace_table)
	{
		//--
		namespace_table.new_enum("RmlEventPhase",
			"None", Rml::EventPhase::None,
			"Capture", Rml::EventPhase::Capture,
			"Target", Rml::EventPhase::Target,
			"Bubble", Rml::EventPhase::Bubble
		);

		namespace_table.new_usertype<Rml::Event>("Event", sol::no_constructor,
			// M
			"StopPropagation", &Rml::Event::StopPropagation,
			//--
			"StopImmediatePropagation", &Rml::Event::StopImmediatePropagation,

			// G+S

			// G
			"current_element", sol::readonly_property(&Rml::Event::GetCurrentElement),
			"type", sol::readonly_property(&Rml::Event::GetType),
			"target_element", sol::readonly_property(&Rml::Event::GetTargetElement),
			"parameters", sol::readonly_property(&functions::getParameters),
			//--
			"event_phase", sol::readonly_property(&Rml::Event::GetPhase),
			"interruptible", sol::readonly_property(&Rml::Event::IsInterruptible),
			"propagating", sol::readonly_property(&Rml::Event::IsPropagating),
			"immediate_propagating", sol::readonly_property(&Rml::Event::IsImmediatePropagating)
		);
	}

} // end namespace Rml::SolLua
