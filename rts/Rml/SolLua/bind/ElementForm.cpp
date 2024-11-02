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

	namespace functions
	{
		template <class TSelf>
		constexpr bool hasAttribute(TSelf& self, const std::string& name)
		{
			return self.HasAttribute(name);
		}
		#define HASATTRGETTER(S, N) [](S& self) { return self.HasAttribute(N); }

		template <class TSelf, typename T>
		T getAttributeWithDefault(TSelf& self, const std::string& name, T def)
		{
			auto attr = self.template GetAttribute<T>(name, def);
			return attr;
		}
		#define GETATTRGETTER(S, N, D) [](S& self) { return functions::getAttributeWithDefault(self, N, D); }

		template <class TSelf, class T>
		constexpr void setAttribute(TSelf& self, const std::string& name, const T& value)
		{
			if constexpr (std::is_same_v<std::decay_t<T>, bool>)
			{
				if (value)
					self.SetAttribute(name, true);
				else self.RemoveAttribute(name);
			}
			else
			{
				self.SetAttribute(name, value);
			}
		}
		#define SETATTR(S, N, D) [](S& self, const D& value) { functions::setAttribute(self, N, value); }
	}

	namespace options
	{
		struct SelectOptionsProxyNode
		{
			Rml::Element* Element;
			std::string Value;
		};

		struct SelectOptionsProxy
		{
			SelectOptionsProxy(Rml::ElementFormControlSelect& element) : m_element(element) {}

			SelectOptionsProxyNode Get(int index) const
			{
				auto element = m_element.GetOption(index);
				if (!element)
					return SelectOptionsProxyNode{ nullptr, {} };

				return SelectOptionsProxyNode{ .Element = element, .Value = element->GetAttribute("value", std::string{}) };
			}

			std::vector<SelectOptionsProxyNode> Pairs() const
			{
				std::vector<SelectOptionsProxyNode> result;
				int i = 0;
				while (auto element = m_element.GetOption(i++))
				{
					result.push_back({ .Element = element, .Value = element->GetAttribute("value", std::string{}) });
				}

				return result;
			}

		private:
			Rml::ElementFormControlSelect& m_element;
		};

		auto getOptionsProxy(Rml::ElementFormControlSelect& self)
		{
			return SelectOptionsProxy{ self };
		}
	}

	namespace submit
	{
		void submit(Rml::ElementForm& self)
		{
			self.Submit();
		}

		void submitName(Rml::ElementForm& self, const std::string& name)
		{
			self.Submit(name);
		}

		void submitNameValue(Rml::ElementForm& self, const std::string& name, const std::string& value)
		{
			self.Submit(name, value);
		}
	}

	void bind_element_form(sol::table& namespace_table)
	{

		namespace_table.new_usertype<Rml::ElementForm>("ElementForm", sol::no_constructor,
			// M
			"Submit", sol::overload(&submit::submit, &submit::submitName, &submit::submitNameValue),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);

		///////////////////////////

		namespace_table.new_usertype<Rml::ElementFormControl>("ElementFormControl", sol::no_constructor,
			// G+S
			"disabled", sol::property(&Rml::ElementFormControl::IsDisabled, &Rml::ElementFormControl::SetDisabled),
			"name", sol::property(&Rml::ElementFormControl::GetName, &Rml::ElementFormControl::SetName),
			"value", sol::property(&Rml::ElementFormControl::GetValue, &Rml::ElementFormControl::SetValue),

			// G
			//--
			"submitted", sol::readonly_property(&Rml::ElementFormControl::IsSubmitted),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);

		///////////////////////////

		namespace_table.new_usertype<Rml::ElementFormControlInput>("ElementFormControlInput", sol::no_constructor,
			// G+S
			"checked", sol::property(HASATTRGETTER(Rml::ElementFormControlInput, "checked"), SETATTR(Rml::ElementFormControlInput, "checked", bool)),
			"maxlength", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "maxlength", -1), SETATTR(Rml::ElementFormControlInput, "maxlength", int)),
			"size", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "size", 20), SETATTR(Rml::ElementFormControlInput, "size", int)),
			"max", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "max", 100), SETATTR(Rml::ElementFormControlInput, "max", int)),
			"min", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "min", 0), SETATTR(Rml::ElementFormControlInput, "min", int)),
			"step", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "step", 1), SETATTR(Rml::ElementFormControlInput, "step", int)),

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControl, Rml::Element>()
		);

		///////////////////////////

		namespace_table.new_usertype<options::SelectOptionsProxy>("SelectOptionsProxy", sol::no_constructor,
			sol::meta_function::index, &options::SelectOptionsProxy::Get,
			sol::meta_function::pairs, &options::SelectOptionsProxy::Pairs,
			sol::meta_function::ipairs, &options::SelectOptionsProxy::Pairs
		);

		namespace_table.new_usertype<options::SelectOptionsProxyNode>("SelectOptionsProxyNode", sol::no_constructor,
			"element", &options::SelectOptionsProxyNode::Element,
			"value", &options::SelectOptionsProxyNode::Value
		);

		namespace_table.new_usertype<Rml::ElementFormControlSelect>("ElementFormControlSelect", sol::no_constructor,
			// M
			"Add", [](Rml::ElementFormControlSelect& self, Rml::ElementPtr& element, sol::variadic_args va) {
				int before = (va.size() > 0 ? va.get<int>() : -1);
				self.Add(std::move(element), before);
				return 1;
			},
			"Remove", &Rml::ElementFormControlSelect::Remove,
			"RemoveAll", &Rml::ElementFormControlSelect::RemoveAll,

			// G+S
			"selection", sol::property(&Rml::ElementFormControlSelect::GetSelection, &Rml::ElementFormControlSelect::SetSelection),

			// G
			"options", &options::getOptionsProxy,

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControl, Rml::Element>()
		);

		///////////////////////////

		namespace_table.new_usertype<Rml::ElementFormControlTextArea>("ElementFormControlTextArea", sol::no_constructor,
			// G+S
			"cols", sol::property(&Rml::ElementFormControlTextArea::GetNumColumns, &Rml::ElementFormControlTextArea::SetNumColumns),
			"maxlength", sol::property(&Rml::ElementFormControlTextArea::GetMaxLength, &Rml::ElementFormControlTextArea::SetMaxLength),
			"rows", sol::property(&Rml::ElementFormControlTextArea::GetNumRows, &Rml::ElementFormControlTextArea::SetNumRows),
			"wordwrap", sol::property(&Rml::ElementFormControlTextArea::SetWordWrap, &Rml::ElementFormControlTextArea::GetWordWrap),

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControl, Rml::Element>()
		);

	}

} // end namespace Rml::SolLua
