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

// Forward declaration for deferred element deletion
extern void AddPendingDelete(Rml::ElementPtr element);

namespace Rml::SolLua
{

	namespace functions
	{
		void addEventListener(Rml::Element& self, const Rml::String& event, sol::protected_function func, const bool in_capture_phase = false)
		{
			auto e = new SolLuaEventListener{ func, &self };
			self.AddEventListener(event, e, in_capture_phase);
		}

		void setInnerRMLSafe(Rml::Element& self, const Rml::String& rml)
		{
			// Manually remove all DOM children and defer their deletion
			// This prevents use-after-free when Lua holds references to children
			while (self.GetNumChildren())
			{
				Rml::Element* child = self.GetChild(0);
				// RemoveChild returns an ElementPtr which owns the child
				Rml::ElementPtr removed = self.RemoveChild(child);
				// Store it for deferred deletion
				AddPendingDelete(std::move(removed));
			}

			// Now set the new content
			if (!rml.empty())
				self.SetInnerRML(rml);
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

		static auto getVisible(Rml::Element& self)
		{
			return self.IsVisible();
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
				if (value.get_type() == sol::type::lua_nil) {
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
		/***
		 * Event listener interface
		 * @class RmlUi.EventListener
		 */
		namespace_table.new_usertype<Rml::EventListener>("EventListener", sol::no_constructor,
			// M
			/***
			 * @function RmlUi.EventListener.OnAttach
			 * @param element RmlUi.Element
			 */
			"OnAttach", &Rml::EventListener::OnAttach,
			/***
			 * @function RmlUi.EventListener.OnDetach
			 * @param element RmlUi.Element
			 */
			"OnDetach", &Rml::EventListener::OnDetach,
			/***
			 * @function RmlUi.EventListener.ProcessEvent
			 * @param event RmlUi.Event
			 */
			"ProcessEvent", &Rml::EventListener::ProcessEvent
		);

		///////////////////////////
		/***
		 * @alias RmlUi.StyleProxy { [string]: string }
		 */
		namespace_table.new_usertype<style::StyleProxy>("StyleProxy", sol::no_constructor,
			sol::meta_function::index, &style::StyleProxy::Get,
			sol::meta_function::new_index, &style::StyleProxy::Set,
			sol::meta_function::pairs, &style::StyleProxy::Pairs
		);

		/***
		 * Represents an element in the RmlUi document tree. This class cannot be constructed directly; use a Document object to instantiate elements. This is the foundational piece of the DOM.
		 * @class RmlUi.Element
		*/



		namespace_table.new_usertype<Rml::Element>("Element", sol::no_constructor,
			// M
			/***
			 * Adds an event listener to the element.
			 * @function RmlUi.Element:AddEventListener
			 * @param event string
			 * @param listener function|string
			 * @param in_capture_phase boolean
			 */
			"AddEventListener", sol::overload(
				[](Rml::Element& s, const Rml::String& e, sol::protected_function f) { functions::addEventListener(s, e, f, false); },
				sol::resolve<void(Rml::Element&, const Rml::String&, sol::protected_function, bool)>(&functions::addEventListener),
				sol::resolve<void(Rml::Element&, const Rml::String&, const Rml::String&, sol::this_state)>(&functions::addEventListener),
				sol::resolve<void(Rml::Element&, const Rml::String&, const Rml::String&, sol::this_state, bool)>(&functions::addEventListener)
			),
			/***
			 * Appends element as a child to this element.
			 * @function RmlUi.Element:AppendChild
			 * @param element RmlUi.ElementPtr
			 * @return RmlUi.ElementPtr
			 */
			"AppendChild", [](Rml::Element& self, Rml::ElementPtr& e) {
				return self.AppendChild(std::move(e));
			},
			/***
			 * Removes input focus from this element.
			 * @function RmlUi.Element:Blur
			 */
			"Blur", &Rml::Element::Blur,
			/***
			 * Fakes a click on this element.
			 * @function RmlUi.Element:Click
			 */
			"Click", &Rml::Element::Click,
			/***
			 * Dispatches an event to this element.
			 * @function RmlUi.Element:DispatchEvent
			 * @param event string
			 * @param parameters table
			 * @param interruptible string
			 * @return boolean
			 */
			"DispatchEvent", sol::resolve<bool(const Rml::String&, const Rml::Dictionary&)>(&Rml::Element::DispatchEvent),
			/***
			 * Gives input focus to this element.
			 * @function RmlUi.Element:Focus
			 */
			"Focus", sol::overload(
				&Rml::Element::Focus,
				[](Rml::Element& self) { self.Focus(true); }
			),
			/***
			 * Returns the value of the attribute named name. If no such attribute exists, the empty string will be returned.
			 * @function RmlUi.Element:GetAttribute
			 * @param name string
			 * @return string?
			 */
			"GetAttribute", &functions::getAttribute,
			/***
			 * Returns the descendant element with an id of id.
			 * @function RmlUi.Element:GetElementById
			 * @param id string
			 * @return RmlUi.Element?
			 */
			"GetElementById", &Rml::Element::GetElementById,
			/***
			 * Returns a list of all descendant elements with the tag of tag_name.
			 * @function RmlUi.Element:GetElementsByTagName
			 * @param tag_name string
			 * @return RmlUi.ElementPtr[]
			 */
			"GetElementsByTagName", &functions::getElementsByTagName,
			/***
			 * JQuery-like element selector
			 * @function RmlUi.Element:QuerySelector
			 * @param query string
			 * @return RmlUi.ElementPtr?
			 */
			"QuerySelector", &Rml::Element::QuerySelector,
			/***
			 * JQuery-like element selector
			 * @function RmlUi.Element:QuerySelectorAll
			 * @param selectors string
			 * @return RmlUi.ElementPtr[]
			 */
			"QuerySelectorAll", &functions::getQuerySelectorAll,
			/***
			 * Checks if the current element matches the given RCSS selector(s).
			 * @function RmlUi.Element:Matches
			 * @param selectors string
			 * @return boolean
			 */
			"Matches", &Rml::Element::Matches,
			/***
			 * Returns True if the element has a value for the attribute named name, False if not.
			 * @function RmlUi.Element:HasAttribute
			 * @param name string
			 * @return boolean
			 */
			"HasAttribute", &Rml::Element::HasAttribute,
			/***
			 * Returns True if the element has at least one child node, false if not.
			 * @function RmlUi.Element:HasChildNodes
			 * @return boolean
			 */
			"HasChildNodes", &Rml::Element::HasChildNodes,
			/***
			 * Inserts the element element as a child of this element, directly before adjacent_element in the list of children.
			 * @function RmlUi.Element:InsertBefore
			 * @param element RmlUi.ElementPtr
			 * @param adjacent_element RmlUi.Element
			 * @return RmlUi.Element?
			 */
			"InsertBefore", [](Rml::Element& self, Rml::ElementPtr& element, Rml::Element* adjacent_element) {
				return self.InsertBefore(std::move(element), adjacent_element);
			},
			/***
			 * Returns true if the class name is set on the element, false if not.
			 * @function RmlUi.Element:IsClassSet
			 * @param name string
			 * @return boolean
			 */
			"IsClassSet", &Rml::Element::IsClassSet,
			/***
			 * Removes the attribute named name from the element.
			 * @function RmlUi.Element:RemoveAttribute
			 * @param name string
			 */
			"RemoveAttribute", &Rml::Element::RemoveAttribute,
			/***
			 * Removes the child element element from this element.
			 * @function RmlUi.Element:RemoveChild
			 * @param element RmlUi.Element
			 * @return boolean
			 */
			"RemoveChild", &Rml::Element::RemoveChild,
			/***
			 * Replaces the child element replaced_element with inserted_element in this element's list of children. If replaced_element is not a child of this element, inserted_element will be appended onto the list instead.
			 * @function RmlUi.Element:ReplaceChild
			 * @param inserted_element RmlUi.ElementPtr
			 * @param replaced_element RmlUi.Element
			 * @return boolean
			 */
			"ReplaceChild", [](Rml::Element& self, Rml::ElementPtr& inserted_element, Rml::Element* replaced_element) {
				return self.ReplaceChild(std::move(inserted_element), replaced_element);
			},
			/***
			 * Scrolls this element into view if its ancestors have hidden overflow.
			 * @function RmlUi.Element:ScrollIntoView
			 * @param align_with_top boolean
			 */
			"ScrollIntoView", [](Rml::Element& self, sol::variadic_args va) {
				if (va.size() == 0)
					self.ScrollIntoView(true);
				else
					self.ScrollIntoView(va[0].as<bool>());
			},
			/***
			 * Sets the value of the attribute named name to value.
			 * @function RmlUi.Element:SetAttribute
			 * @param name string
			 * @param value string
			 */
			"SetAttribute", static_cast<void(Rml::Element::*)(const Rml::String&, const Rml::String&)>(&Rml::Element::SetAttribute),
			/***
			 * Sets (if value is true) or clears (if value is false) the class name on the element.
			 * @function RmlUi.Element:SetClass
			 * @param name string
			 * @param value boolean
			 */
			"SetClass", &Rml::Element::SetClass,
			//--
			/***
			 * @function RmlUi.Element:GetElementsByClassName
			 * @param class_name string
			 * @return RmlUi.Element[]
			 */
			"GetElementsByClassName", &functions::getElementsByClassName,
			/***
			 * @function RmlUi.Element:Clone
			 * @return RmlUi.ElementPtr
			 */
			"Clone", &Rml::Element::Clone,
			/***
			 * @function RmlUi.Element:Closest
			 * @return RmlUi.Element?
			 */
			"Closest", &Rml::Element::Closest,
			/***
			 * @function RmlUi.Element:SetPseudoClass
			 * @param class_name string
			 */
			"SetPseudoClass", &Rml::Element::SetPseudoClass,
			/***
			 * @function RmlUi.Element:IsPseudoClassSet
			 * @param class_name string
			 * @return boolean
			 */
			"IsPseudoClassSet", &Rml::Element::IsPseudoClassSet,
			/***
			 * @function RmlUi.Element:ArePseudoCLassesSet
			 * @param class_names string[]
			 * @return boolean
			 */
			"ArePseudoClassesSet", &Rml::Element::ArePseudoClassesSet,
			/***
			 * @function RmlUi.Element:GetActivePseudoCLasses
			 * @return string[]
			 */
			"GetActivePseudoClasses", &Rml::Element::GetActivePseudoClasses,
			/***
			 * Is a screen-space point within this element?
			 * @function RmlUi.Element:IsPointWithinElement
			 * @param point RmlUi.Vector2f
			 * @return boolean
			 */
			"IsPointWithinElement", &Rml::Element::IsPointWithinElement,
			/***
			 * @function RmlUi.Element:ProcessDefaultAction
			 * @param event RmlUi.Event
			 */
			"ProcessDefaultAction", &Rml::Element::ProcessDefaultAction,
			/***
			 * @function RmlUi.Element:IsVisible
			 * True if the element is visible, false otherwise.
			 * @return boolean
			 */
			"IsVisible", sol::overload(
				&functions::getVisible,
				&Rml::Element::IsVisible
			),
			/***
			 * Get the value of this element.
			 * @function RmlUi.Element:GetValue
			 * @return number | string | "" value Returns number if it has the tag "input", a string if it has the tag "textarea", else an empty string.
			 */
			"GetValue",[](Rml::Element& self) {
				if (self.GetTagName() == "input") {
					return dynamic_cast<Rml::ElementFormControlInput*>(&self)->GetValue();
				} else if (self.GetTagName() == "textarea") {
					return dynamic_cast<Rml::ElementFormControlTextArea*>(&self)->GetValue();
				}
				return std::string();
			},
			/***
			 * @function RmlUi.Element:GetChild
			 * @param index integer
			 * @return RmlUi.Element?
			 */
			"GetChild", [](Rml::Element& self, int index) { return self.GetChild(index); },

			// G+S
			/*** @field RmlUi.Element.class_name string Name of the class. */
			"class_name", sol::property(&Rml::Element::GetClassNames, &Rml::Element::SetClassNames),
			/*** @field RmlUi.Element.id string ID of this element, in the context of `<span id="foo">`. */
			"id", sol::property(&Rml::Element::GetId, &Rml::Element::SetId),
			/*** @field RmlUi.Element.inner_rml string Gets or sets the inner RML (markup) content of the element. */
			"inner_rml", sol::property(sol::resolve<Rml::String() const>(&Rml::Element::GetInnerRML), &functions::setInnerRMLSafe),
			/*** @field RmlUi.Element.scroll_left integer Gets or sets the number of pixels that the content of the element is scrolled from the left. */
			"scroll_left", sol::property(&Rml::Element::GetScrollLeft, &Rml::Element::SetScrollLeft),
			/*** @field RmlUi.Element.scroll_top integer Gets or sets the number of pixels that the content of the element is scrolled from the top. */
			"scroll_top", sol::property(&Rml::Element::GetScrollTop, &Rml::Element::SetScrollTop),

			// G
			/*** @field RmlUi.Element.attributes RmlUi.ElementAttributesProxy Read-only. Proxy for accessing element attributes. */
			"attributes", sol::readonly_property(&functions::getAttributes),
			/*** @field RmlUi.Element.child_nodes RmlUi.ElementChildNodesProxy Read-only. Proxy for accessing child nodes of the element. */
			"child_nodes", sol::readonly_property(&getIndexedTable<Rml::Element, Rml::Element, &Rml::Element::GetChild, &child::getMaxChildren>),
			/*** @field RmlUi.Element.client_left integer Read-only. The width of the left border of the element in pixels. */
			"client_left", sol::readonly_property(&Rml::Element::GetClientLeft),
			/*** @field RmlUi.Element.client_height integer Read-only. The inner height of the element in pixels, including padding but not the horizontal scrollbar height, border, or margin. */
			"client_height", sol::readonly_property(&Rml::Element::GetClientHeight),
			/*** @field RmlUi.Element.client_top integer Read-only. The width of the top border of the element in pixels. */
			"client_top", sol::readonly_property(&Rml::Element::GetClientTop),
			/*** @field RmlUi.Element.client_width integer Read-only. The inner width of the element in pixels, including padding but not the vertical scrollbar width, border, or margin. */
			"client_width", sol::readonly_property(&Rml::Element::GetClientWidth),
			/*** @field RmlUi.Element.first_child RmlUi.Element? Read-only. The first child element, or nil if there are no children. */
			"first_child", sol::readonly_property(&Rml::Element::GetFirstChild),
			/*** @field RmlUi.Element.last_child RmlUi.Element? Read-only. The last child element, or nil if there are no children. */
			"last_child", sol::readonly_property(&Rml::Element::GetLastChild),
			/*** @field RmlUi.Element.next_sibling RmlUi.Element? Read-only. The next sibling element, or nil if there is none. */
			"next_sibling", sol::readonly_property(&Rml::Element::GetNextSibling),
			/*** @field RmlUi.Element.offset_height integer Read-only. The height of the element including vertical padding and borders, in pixels. */
			"offset_height", sol::readonly_property(&Rml::Element::GetOffsetHeight),
			/*** @field RmlUi.Element.offset_left integer Read-only. The distance from the inner left edge of the offset parent, in pixels. */
			"offset_left", sol::readonly_property(&Rml::Element::GetOffsetLeft),
			/*** @field RmlUi.Element.offset_parent RmlUi.Element Read-only. The closest positioned ancestor element. */
			"offset_parent", sol::readonly_property(&Rml::Element::GetOffsetParent),
			/*** @field RmlUi.Element.offset_top integer Read-only. The distance from the inner top edge of the offset parent, in pixels. */
			"offset_top", sol::readonly_property(&Rml::Element::GetOffsetTop),
			/*** @field RmlUi.Element.offset_width integer Read-only. The width of the element including horizontal padding and borders, in pixels. */
			"offset_width", sol::readonly_property(&Rml::Element::GetOffsetWidth),
			/*** @field RmlUi.Element.owner_document RmlUi.Document Read-only. The document that owns this element. */
			"owner_document", sol::readonly_property(&functions::getOwnerDocument),
			/*** @field RmlUi.Element.parent_node RmlUi.Element? Read-only. The parent node of this element, or nil if there is none. */
			"parent_node", sol::readonly_property(&Rml::Element::GetParentNode),
			/*** @field RmlUi.Element.previous_sibling RmlUi.Element? Read-only. The previous sibling element, or nil if there is none. */
			"previous_sibling", sol::readonly_property(&Rml::Element::GetPreviousSibling),
			/*** @field RmlUi.Element.scroll_height integer Read-only. The total height of the element's content, including content not visible on the screen due to overflow. */
			"scroll_height", sol::readonly_property(&Rml::Element::GetScrollHeight),
			/*** @field RmlUi.Element.scroll_width integer Read-only. The total width of the element's content, including content not visible on the screen due to overflow. */
			"scroll_width", sol::readonly_property(&Rml::Element::GetScrollWidth),
			/*** @field RmlUi.Element.style RmlUi.ElementStyleProxy Read-only. Proxy for accessing and modifying the element's style properties. */
			"style", sol::readonly_property(&style::getElementStyleProxy),
			/*** @field RmlUi.Element.tag_name string Read-only. The tag name of the element. */
			"tag_name", sol::readonly_property(&Rml::Element::GetTagName),
			//--
			/*** @field RmlUi.Element.address string Read-only. The address of the element in the document tree. */
			"address", sol::readonly_property([](Rml::Element& self) { return self.GetAddress(); }),
			/*** @field RmlUi.Element.absolute_left integer Read-only. The absolute left position of the element relative to the document. */
			"absolute_left", sol::readonly_property(&Rml::Element::GetAbsoluteLeft),
			/*** @field RmlUi.Element.absolute_top integer Read-only. The absolute top position of the element relative to the document. */
			"absolute_top", sol::readonly_property(&Rml::Element::GetAbsoluteTop),
			/*** @field RmlUi.Element.baseline integer Read-only. The baseline position of the element. */
			"baseline", sol::readonly_property(&Rml::Element::GetBaseline),
			/*** @field RmlUi.Element.line_height integer Read-only. The computed line height of the element. */
			"line_height", sol::readonly_property(&Rml::Element::GetLineHeight),
			/*** @field RmlUi.Element.visible boolean Read-only. True if the element is visible, false otherwise. */
			"visible", sol::readonly_property(&functions::getVisible),
			/*** @field RmlUi.Element.z_index integer Read-only. The computed z-index of the element. */
			"z_index", sol::readonly_property(&Rml::Element::GetZIndex)
		);

	}

} // end namespace Rml::SolLua
