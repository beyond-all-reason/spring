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

	namespace input
	{
		static auto getSelection(Rml::ElementFormControlInput& self)
		{
			int start, end;
			Rml::String text;
			self.GetSelection(&start, &end, &text);
			return std::make_tuple(start, end, text);
		}
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

	namespace textarea
	{
		static auto getSelection(Rml::ElementFormControlTextArea& self)
		{
			int start, end;
			Rml::String text;
			self.GetSelection(&start, &end, &text);
			return std::make_tuple(start, end, text);
		}
	}

	void bind_element_form(sol::table& namespace_table)
	{
		/***
		 * @class RmlUi.ElementForm : RmlUi.Element
		 */
		namespace_table.new_usertype<Rml::ElementForm>("ElementForm", sol::no_constructor,
			// M
			/***
			 * @function RmlUi.ElementForm:Submit
			 * @param name string?
			 * @param value string?
			 */
			"Submit", sol::overload(&submit::submit, &submit::submitName, &submit::submitNameValue),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);

		///////////////////////////
		/***
		 * @class RmlUi.ElementFormControl : RmlUi.Element
		 */
		namespace_table.new_usertype<Rml::ElementFormControl>("ElementFormControl", sol::no_constructor,
			// G+S
			/*** @field RmlUi.ElementFormControl.disabled boolean*/
			"disabled", sol::property(&Rml::ElementFormControl::IsDisabled, &Rml::ElementFormControl::SetDisabled),
			/*** @field RmlUi.ElementFormControl.name string*/
			"name", sol::property(&Rml::ElementFormControl::GetName, &Rml::ElementFormControl::SetName),
			/*** @field RmlUi.ElementFormControl.value string*/
			"value", sol::property(&Rml::ElementFormControl::GetValue, &Rml::ElementFormControl::SetValue),

			// G
			//--
			/*** @field RmlUi.ElementFormControl.submitted boolean*/
			"submitted", sol::readonly_property(&Rml::ElementFormControl::IsSubmitted),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);

		///////////////////////////
		/***
		 * @class RmlUi.ElementFormControlInput : RmlUi.Element, RmlUi.ElementFormControl
		 */
		namespace_table.new_usertype<Rml::ElementFormControlInput>("ElementFormControlInput", sol::no_constructor,
			// M
			/***
			 * @function RmlUi.ElementFormControlInput:Select
			 * Selects all text.
			 */
			"Select", &Rml::ElementFormControlInput::Select,
			/***
			 * @function RmlUi.ElementFormControlInput:SetSelection
			 * Selects the text in the given character range.
			 * @param selection_start integer The first character to be selected.
			 * @param selection_end integer The first character *after* the selection.
			 */
			"SetSelection", &Rml::ElementFormControlInput::SetSelectionRange,
			/***
			 * @function RmlUi.ElementFormControlInput:GetSelection
			 * Retrieves the selection range and text.
			 * @return any selection_start The first character selected; selection_end The first character *after* the selection; selected_text The selected text.
			 */
			"GetSelection", &input::getSelection,

			// G+S
			/*** @field RmlUi.ElementFormControlInput.checked boolean */
			"checked", sol::property(HASATTRGETTER(Rml::ElementFormControlInput, "checked"), SETATTR(Rml::ElementFormControlInput, "checked", bool)),
			/*** @field RmlUi.ElementFormControlInput.maxlength integer */
			"maxlength", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "maxlength", -1), SETATTR(Rml::ElementFormControlInput, "maxlength", int)),
			/*** @field RmlUi.ElementFormControlInput.size integer */
			"size", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "size", 20), SETATTR(Rml::ElementFormControlInput, "size", int)),
			/*** @field RmlUi.ElementFormControlInput.max integer */
			"max", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "max", 100), SETATTR(Rml::ElementFormControlInput, "max", int)),
			/*** @field RmlUi.ElementFormControlInput.min integer */
			"min", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "min", 0), SETATTR(Rml::ElementFormControlInput, "min", int)),
			/*** @field RmlUi.ElementFormControlInput.step integer */
			"step", sol::property(GETATTRGETTER(Rml::ElementFormControlInput, "step", 1), SETATTR(Rml::ElementFormControlInput, "step", int)),

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControl, Rml::Element>()
		);

		///////////////////////////
		/***
		 * @alias RmlUi.SelectOptionsProxy RmlUi.SelectOptionsProxyNode[]
		 */
		namespace_table.new_usertype<options::SelectOptionsProxy>("SelectOptionsProxy", sol::no_constructor,
			sol::meta_function::index, &options::SelectOptionsProxy::Get,
			sol::meta_function::pairs, &options::SelectOptionsProxy::Pairs,
			sol::meta_function::ipairs, &options::SelectOptionsProxy::Pairs
		);
		/***
		 * @alias RmlUi.SelectOptionsProxyNode {element: RmlUi.Element, value: string}
		 */
		namespace_table.new_usertype<options::SelectOptionsProxyNode>("SelectOptionsProxyNode", sol::no_constructor,
			"element", &options::SelectOptionsProxyNode::Element,
			"value", &options::SelectOptionsProxyNode::Value
		);
		/***
		 * @class RmlUi.ElementFormControlSelect : RmlUi.Element, RmlUi.ElementFormControl
		 */
		namespace_table.new_usertype<Rml::ElementFormControlSelect>("ElementFormControlSelect", sol::no_constructor,
			// M
			/***
			 * @function RmlUi.ElementFormControlSelect:Add
			 * @param element RmlUi.Element
			 * @param before integer?
			 */
			"Add", [](Rml::ElementFormControlSelect& self, Rml::ElementPtr& element, sol::variadic_args va) {
				int before = (va.size() > 0 ? va.get<int>() : -1);
				self.Add(std::move(element), before);
				return 1;
			},
			/***
			 * @function RmlUi.ElementFormControlSelect:Remove
			 * @param index integer
			 */
			"Remove", &Rml::ElementFormControlSelect::Remove,
			/***
			 * @function RmlUi.ElementFormControlSelect:RemoveAll
			 */
			"RemoveAll", &Rml::ElementFormControlSelect::RemoveAll,

			// G+S
			/*** @field RmlUi.ElementsFormControlSelect.selection integer */
			"selection", sol::property(&Rml::ElementFormControlSelect::GetSelection, &Rml::ElementFormControlSelect::SetSelection),

			// G
			/*** @field RmlUi.ElementsFormControlSelect.options RmlUi.SelectOptionsProxy */
			"options", &options::getOptionsProxy,

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControl, Rml::Element>()
		);

		///////////////////////////
		/***
		 * @class RmlUi.ElementFormControlTextArea : RmlUi.Element, RmlUi.ElementFormControl
		 */
		namespace_table.new_usertype<Rml::ElementFormControlTextArea>("ElementFormControlTextArea", sol::no_constructor,
			// M
			/***
			 * @function RmlUi.ElementFormControlTextArea:Select
			 * Selects all text.
			 */
			"Select", &Rml::ElementFormControlTextArea::Select,
			/***
			 * @function RmlUi.ElementFormControlTextArea:SetSelection
			 * Selects the text in the given character range.
			 * @param selection_start integer The first character to be selected.
			 * @param selection_end integer The first character *after* the selection.
			 */
			"SetSelection", &Rml::ElementFormControlTextArea::SetSelectionRange,
			/***
			 * @function RmlUi.ElementFormControlTextArea:GetSelection
			 * Retrieves the selection range and text.
			 * @return any selection_start The first character selected; selection_end The first character *after* the selection; selected_text The selected text.
			 */
			"GetSelection", &textarea::getSelection,

			// G+S
			/*** @field RmlUi.ElementFormControlTextArea.cols integer */
			"cols", sol::property(&Rml::ElementFormControlTextArea::GetNumColumns, &Rml::ElementFormControlTextArea::SetNumColumns),
			/*** @field RmlUi.ElementFormControlTextArea.maxlength integer */
			"maxlength", sol::property(&Rml::ElementFormControlTextArea::GetMaxLength, &Rml::ElementFormControlTextArea::SetMaxLength),
			/*** @field RmlUi.ElementFormControlTextArea.rows integer */
			"rows", sol::property(&Rml::ElementFormControlTextArea::GetNumRows, &Rml::ElementFormControlTextArea::SetNumRows),
			/*** @field RmlUi.ElementFormControlTextArea.wordwrap boolean */
			"wordwrap", sol::property(&Rml::ElementFormControlTextArea::SetWordWrap, &Rml::ElementFormControlTextArea::GetWordWrap),

			// B
			sol::base_classes, sol::bases<Rml::ElementFormControl, Rml::Element>()
		);

	}

} // end namespace Rml::SolLua
