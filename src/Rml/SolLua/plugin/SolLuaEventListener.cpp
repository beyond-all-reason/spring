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

#include "SolLuaEventListener.h"

#include "SolLuaDocument.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <string>

namespace Rml::SolLua
{

	SolLuaEventListener::SolLuaEventListener(sol::state_view& lua, const Rml::String& code, Rml::Element* element)
		: m_element(element)
	{
		if (element == nullptr)
			return;

		auto* context = element->GetContext();
		auto* document = element->GetOwnerDocument();

		// Wrap our code so we pass event, element, and document.
		//auto f = std::format("return function (event,element,document) {} end", code);
		Rml::String f{ "--" };
		if (context != nullptr)
		{
			f.append("[");
			f.append(context->GetName());
			f.append("]");
		}
		if (document != nullptr)
		{
			f.append("[");
			f.append(document->GetSourceURL());
			f.append("]");
		}

		if (f.size() != 2)
			f.append(" ");

		f.append(element->GetTagName());
		if (const auto& id = element->GetId(); !id.empty())
		{
			f.append("#");
			f.append(id);
		}
		if (auto classes = element->GetClassNames(); !classes.empty())
		{
			f.append(".");
			std::replace(classes.begin(), classes.end(), ' ', '.');
			f.append(classes);
		}

		f.append("\n");
		f.append("return function (event,element,document) ");
		f.append(code);
		f.append(" end");

		// Run the script and get our function to call.
		// We would have liked to call SolLuaDocument::RunLuaScript, but we don't know our owner_document at this point!
		// Just get the function now.  When we process the event, we will move it to the environment.
		auto result = lua.safe_script(f, ErrorHandler);
		if (result.valid())
		{
			auto obj = result.get<sol::object>();
			if (obj.get_type() == sol::type::function)
			{
				auto func = obj.as<sol::protected_function>();
				m_func = func;
			}
			else
			{
				Log::Message(Log::LT_ERROR, "[Lua] A function wasn't returned for the event listener.");
			}
		}
	}

	SolLuaEventListener::SolLuaEventListener(sol::protected_function func, Rml::Element* element)
		: m_func(func), m_element(element)
	{
	}

	void SolLuaEventListener::OnDetach(Rml::Element* element)
	{
		delete this;
	}

	void SolLuaEventListener::ProcessEvent(Rml::Event& event)
	{
		auto document = dynamic_cast<SolLuaDocument*>(m_element->GetOwnerDocument());
		if (document != nullptr && m_func.valid())
		{
			auto& env = document->GetLuaEnvironment();
			const auto& ident = document->GetLuaEnvironmentIdentifier();

			// Move our event into the Lua environment.
			sol::set_environment(env, m_func);

			// If we have an identifier, set it now.
			if (!ident.empty())
				env.set(ident, document->GetId());

			// Call the event!
			auto result = m_func.call(event, m_element, document);
			if (!result.valid()){
				ErrorHandler(m_func.lua_state(), std::move(result));
			}
		}
	}

} // namespace Rml::SolLua
