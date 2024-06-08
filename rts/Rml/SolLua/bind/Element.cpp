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

#include "../plugin/SolLuaDocument.h"
#include "../plugin/SolLuaEventListener.h"

#include <unordered_map>


namespace Rml::SolLua
{

	namespace functions
	{
		void addEventListener(Rml::Element& self, const Rml::String& event, sol::protected_function func, const bool in_capture_phase = false)
		{
			auto e = new SolLuaEventListener{ func, &self };
			self.AddEventListener(event, e, in_capture_phase);
		}

		void addEventListener(Rml::Element& self, const Rml::String& event, const Rml::String& code, sol::this_state s)
		{
			auto state = sol::state_view{ s };
			auto e = new SolLuaEventListener{ state, code, &self };
			self.AddEventListener(event, e, false);
		}

		void addEventListener(Rml::Element& self, const Rml::String& event, const Rml::String& code, sol::this_state s, const bool in_capture_phase)
		{
			auto state = sol::state_view{ s };
			auto e = new SolLuaEventListener{ state, code, &self };
			self.AddEventListener(event, e, in_capture_phase);
		}

		auto getAttribute(Rml::Element& self, const Rml::String& name, sol::this_state s)
		{
			sol::state_view state{ s };

			auto attr = self.GetAttribute(name);
			return makeObjectFromVariant(attr, s);
		}

		auto getElementsByTagName(Rml::Element& self, const Rml::String& tag)
		{
			Rml::ElementList result;
			self.GetElementsByTagName(result, tag);
			return result;
		}

		auto getElementsByClassName(Rml::Element& self, const Rml::String& class_name)
		{
			Rml::ElementList result;
			self.GetElementsByClassName(result, class_name);
			return result;
		}

		auto getAttributes(Rml::Element& self, sol::this_state s)
		{
			SolObjectMap result;

			const auto& attributes = self.GetAttributes();
			for (auto& [key, value] : attributes)
			{
				auto object = makeObjectFromVariant(&value, s);
				result.insert(std::make_pair(key, object));
			}

			return result;
		}

		auto getOwnerDocument(Rml::Element& self)
		{
			auto document = self.GetOwnerDocument();
			auto soldocument = dynamic_cast<SolLuaDocument*>(document);
			return soldocument;
		}

		auto getQuerySelectorAll(Rml::Element& self, const Rml::String& selector)
		{
			Rml::ElementList result;
			self.QuerySelectorAll(result, selector);
			return result;
		}
	}

	namespace child
	{
		auto getMaxChildren(Rml::Element& self)
		{
			std::function<int()> result = std::bind(&Rml::Element::GetNumChildren, &self, false);
			return result;
		}
	}

	namespace style
	{
		auto nextPair(sol::user<Rml::PropertiesIteratorView&> iter_state, sol::this_state s)
		{
			auto& iter = iter_state.value();

			if (iter.AtEnd())
				return std::make_tuple(sol::object(sol::lua_nil), sol::object(sol::lua_nil));

			auto result = std::make_tuple(
				sol::object(s, sol::in_place, iter.GetName()),
				sol::object(s, sol::in_place, iter.GetProperty().ToString())
			);
			++iter;

			return result;
		}

		struct StyleProxy
		{
			explicit StyleProxy(Rml::Element* element) : m_element(element) {}

			std::string Get(const std::string& name)
			{
				auto prop = m_element->GetProperty(name);
				if (prop == nullptr) return {};
				return prop->ToString();
			}

			void Set(const sol::this_state L, const std::string& name, const sol::object& value)
			{
				if (value.get_type() == sol::type::nil) {
					m_element->RemoveProperty(name);
					return;
				}

				if (value.get_type() == sol::type::string) {
					auto str = value.as<std::string&>();

					if (str.empty()) {
						m_element->RemoveProperty(name);
					} else {
						m_element->SetProperty(name, str);
					}

					return;
				}

				sol::type_error(L, sol::type::string, value.get_type());
			}

			auto Pairs()
			{
				auto iter = m_element->IterateLocalProperties();
				return std::make_tuple(
					&nextPair,
					sol::user<Rml::PropertiesIteratorView>(std::move(iter)),
					sol::lua_nil
				);
			}

		private:
			Rml::Element* m_element;
		};

		auto getElementStyleProxy(Rml::Element* self)
		{
			return StyleProxy{ self };
		}
	}

	void bind_element(sol::table& namespace_table)
	{
		namespace_table.new_usertype<Rml::EventListener>("EventListener", sol::no_constructor,
			// M
			"OnAttach", &Rml::EventListener::OnAttach,
			"OnDetach", &Rml::EventListener::OnDetach,
			"ProcessEvent", &Rml::EventListener::ProcessEvent
		);

		///////////////////////////

		namespace_table.new_usertype<style::StyleProxy>("StyleProxy", sol::no_constructor,
														sol::meta_function::index, &style::StyleProxy::Set,
			sol::meta_function::new_index, &style::StyleProxy::Set,
			sol::meta_function::pairs, &style::StyleProxy::Pairs
		);

		namespace_table.new_usertype<Rml::Element>("Element", sol::no_constructor,
			// M
			"AddEventListener", sol::overload(
				[](Rml::Element& s, const Rml::String& e, sol::protected_function f) { functions::addEventListener(s, e, f, false); },
				sol::resolve<void(Rml::Element&, const Rml::String&, sol::protected_function, bool)>(&functions::addEventListener),
				sol::resolve<void(Rml::Element&, const Rml::String&, const Rml::String&, sol::this_state)>(&functions::addEventListener),
				sol::resolve<void(Rml::Element&, const Rml::String&, const Rml::String&, sol::this_state, bool)>(&functions::addEventListener)
			),
			"AppendChild", [](Rml::Element& self, Rml::ElementPtr& e) {
				return self.AppendChild(std::move(e));
			},
			"Blur", &Rml::Element::Blur,
			"Click", &Rml::Element::Click,
			"DispatchEvent", sol::resolve<bool(const Rml::String&, const Rml::Dictionary&)>(&Rml::Element::DispatchEvent),
			"Focus", &Rml::Element::Focus,
			"GetAttribute", &functions::getAttribute,
			"GetElementById", &Rml::Element::GetElementById,
			"GetElementsByTagName", &functions::getElementsByTagName,
			"QuerySelector", &Rml::Element::QuerySelector,
			"QuerySelectorAll", &functions::getQuerySelectorAll,
			"HasAttribute", &Rml::Element::HasAttribute,
			"HasChildNodes", &Rml::Element::HasChildNodes,
			"InsertBefore", [](Rml::Element& self, Rml::ElementPtr& element, Rml::Element* adjacent_element) {
				return self.InsertBefore(std::move(element), adjacent_element);
			},
			"IsClassSet", &Rml::Element::IsClassSet,
			"RemoveAttribute", &Rml::Element::RemoveAttribute,
			"RemoveChild", &Rml::Element::RemoveChild,
			"ReplaceChild", [](Rml::Element& self, Rml::ElementPtr& inserted_element, Rml::Element* replaced_element) {
				return self.ReplaceChild(std::move(inserted_element), replaced_element);
			},
			"ScrollIntoView", [](Rml::Element& self, sol::variadic_args va) {
				if (va.size() == 0)
					self.ScrollIntoView(true);
				else
					self.ScrollIntoView(va[0].as<bool>());
			},
			"SetAttribute", static_cast<void(Rml::Element::*)(const Rml::String&, const Rml::String&)>(&Rml::Element::SetAttribute),
			"SetClass", &Rml::Element::SetClass,
			//--
			"GetElementsByClassName", &functions::getElementsByClassName,
			"Clone", &Rml::Element::Clone,
			"Closest", &Rml::Element::Closest,
			"SetPseudoClass", &Rml::Element::SetPseudoClass,
			"IsPseudoClassSet", &Rml::Element::IsPseudoClassSet,
			"ArePseudoClassesSet", &Rml::Element::ArePseudoClassesSet,
			"GetActivePseudoClasses", &Rml::Element::GetActivePseudoClasses,
			"IsPointWithinElement", &Rml::Element::IsPointWithinElement,
			"ProcessDefaultAction", &Rml::Element::ProcessDefaultAction,
			"GetValue",[](Rml::Element& self) {
				if (self.GetTagName() == "input") {
					return dynamic_cast<Rml::ElementFormControlInput*>(&self)->GetValue();
				} else if (self.GetTagName() == "textarea") {
					return dynamic_cast<Rml::ElementFormControlTextArea*>(&self)->GetValue();
				}
				return std::string();
			},
			"GetChild", [](Rml::Element& self, int index) { return self.GetChild(index); },

			// G+S
			"class_name", sol::property(&Rml::Element::GetClassNames, &Rml::Element::SetClassNames),
			"id", sol::property(&Rml::Element::GetId, &Rml::Element::SetId),
			"inner_rml", sol::property(sol::resolve<Rml::String() const>(&Rml::Element::GetInnerRML), &Rml::Element::SetInnerRML),
			"scroll_left", sol::property(&Rml::Element::GetScrollLeft, &Rml::Element::SetScrollLeft),
			"scroll_top", sol::property(&Rml::Element::GetScrollTop, &Rml::Element::SetScrollTop),

			// G
			"attributes", sol::readonly_property(&functions::getAttributes),
			"child_nodes", sol::readonly_property(&getIndexedTable<Rml::Element, Rml::Element, &Rml::Element::GetChild, &child::getMaxChildren>),
			"client_left", sol::readonly_property(&Rml::Element::GetClientLeft),
			"client_height", sol::readonly_property(&Rml::Element::GetClientHeight),
			"client_top", sol::readonly_property(&Rml::Element::GetClientTop),
			"client_width", sol::readonly_property(&Rml::Element::GetClientWidth),
			"first_child", sol::readonly_property(&Rml::Element::GetFirstChild),
			"last_child", sol::readonly_property(&Rml::Element::GetLastChild),
			"next_sibling", sol::readonly_property(&Rml::Element::GetNextSibling),
			"offset_height", sol::readonly_property(&Rml::Element::GetOffsetHeight),
			"offset_left", sol::readonly_property(&Rml::Element::GetOffsetLeft),
			"offset_parent", sol::readonly_property(&Rml::Element::GetOffsetParent),
			"offset_top", sol::readonly_property(&Rml::Element::GetOffsetTop),
			"offset_width", sol::readonly_property(&Rml::Element::GetOffsetWidth),
			"owner_document", sol::readonly_property(&functions::getOwnerDocument),
			"parent_node", sol::readonly_property(&Rml::Element::GetParentNode),
			"previous_sibling", sol::readonly_property(&Rml::Element::GetPreviousSibling),
			"scroll_height", sol::readonly_property(&Rml::Element::GetScrollHeight),
			"scroll_width", sol::readonly_property(&Rml::Element::GetScrollWidth),
			"style", sol::readonly_property(&style::getElementStyleProxy),
			"tag_name", sol::readonly_property(&Rml::Element::GetTagName),
			//--
			"address", sol::readonly_property([](Rml::Element& self) { return self.GetAddress(); }),
			"absolute_left", sol::readonly_property(&Rml::Element::GetAbsoluteLeft),
			"absolute_top", sol::readonly_property(&Rml::Element::GetAbsoluteTop),
			"baseline", sol::readonly_property(&Rml::Element::GetBaseline),
			"line_height", sol::readonly_property(&Rml::Element::GetLineHeight),
			"visible", sol::readonly_property(&Rml::Element::IsVisible),
			"z_index", sol::readonly_property(&Rml::Element::GetZIndex)
		);

	}

} // end namespace Rml::SolLua
