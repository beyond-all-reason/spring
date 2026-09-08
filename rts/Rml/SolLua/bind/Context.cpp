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
#include "../plugin/SolLuaDocument.h"
#include "Rml/Backends/RmlUi_Backend.h"
#include "sol2/sol.hpp"

#include <memory>
#include <string>
#include <variant>

namespace Rml::SolLua
{

namespace document
{
/// <summary>
/// Return a SolLuaDocument.
/// </summary>
auto getDocumentBypass(Rml::Context& self, int idx)
{
	auto document = self.GetDocument(idx);
	auto result = dynamic_cast<SolLuaDocument*>(document);
	return result;
}

/// <summary>
/// Return a SolLuaDocument.
/// </summary>
auto getDocumentBypassString(Rml::Context& self, const Rml::String& name)
{
	auto document = self.GetDocument(name);
	return dynamic_cast<SolLuaDocument*>(document);
}

/// <summary>
/// Helper function to fill the indexed table with data.
/// </summary>
auto getDocument(Rml::Context& self)
{
	std::function<SolLuaDocument*(int)> result = [&self](int idx) -> auto {
		return getDocumentBypass(self, idx);
	};
	return result;
}
}  // namespace document

namespace datamodel
{
/// <summary>
/// Bind a sol::table into the data model.
/// </summary>
/// <param name="data">The data model container.</param>
/// <param name="table">The table to bind.</param>
void bindTable(SolLuaDataModel* data, sol::table& table)
{
	for (auto& [key, value] : table) {
		std::string skey;
		if (key.is<int>()) {
			skey = std::to_string(key.as<int>());
		} else {
			skey = key.as<std::string>();
		}

		if (value.get_type() == sol::type::function) {
			data->Constructor.BindEventCallback(skey,
				[skey, cb = sol::protected_function{value}, state = sol::state_view{table.lua_state()}]
				(Rml::DataModelHandle, Rml::Event& event, const Rml::VariantList& varlist)
			{
				if (cb.valid()) {
					std::vector<sol::object> args;
					for (const auto& variant : varlist) {
						args.push_back(makeObjectFromVariant(&variant, state));
					}
					auto pfr = cb(event, sol::as_args(args));
					if (!pfr.valid()) {
						ErrorHandler(cb.lua_state(), std::move(pfr));
					}
				}
			});
			data->BindingMap[skey] = SolLuaDataModel::BindingType::Function;
		} else {
			auto it = data->ObjectMap.insert_or_assign(skey, DataVariableReference(table, skey, ""));
			data->Constructor.BindCustomDataVariable(skey, Rml::DataVariable(data->ObjectDef.get(), &(it.first->second)));
			data->BindingMap[skey] = SolLuaDataModel::BindingType::Variable;
		}
	}
}

struct lua_iterator_state
{
	std::vector<sol::object> keys;
	std::vector<sol::object>::iterator it;
	std::vector<sol::object>::iterator last;
	sol::table valuetable;

	lua_iterator_state(sol::table keytable, sol::table valuetable, sol::this_state s, bool ipairs = false)
		: valuetable(valuetable)
	{
		if (ipairs) {
			sol::state_view l{s};
			int index = 0;
			int count = keytable.size();
			while (keytable.get<sol::object>(++index).get_type() != sol::type::lua_nil && index <= count) {
				this->keys.emplace_back(sol::object(l, sol::in_place, index));
			}
		} else {
			keytable.for_each([this](sol::object key, sol::object value) {
				this->keys.emplace_back(key);
			});
		}
		it = keys.begin();
		last = keys.end();
	}
};

auto nextPair(sol::user<lua_iterator_state&> user_it_state, sol::this_state l)
{
	lua_iterator_state& it_state = user_it_state;
	auto& it = it_state.it;
	if (it == it_state.last) {
		return std::make_tuple(sol::object(sol::lua_nil), sol::object(sol::lua_nil));
	}
	auto& key = *it;
	auto r = std::make_tuple(key, it_state.valuetable.get<sol::object>(key));
	std::advance(it, 1);
	return r;
}

constexpr auto createPairsFunction(bool ipairs = false)
{
	return ([ipairs] (sol::object self, sol::this_state s) {
			auto metatable = self.as<sol::table>();
			auto rawtable = metatable.get<sol::function>("__raw").call<sol::table>();
			return std::make_tuple(&nextPair, sol::user<lua_iterator_state>(rawtable, metatable, s, ipairs), sol::lua_nil);
		});
}

sol::object getFromTable(const sol::table& t, const std::vector<std::variant<int, std::string>>& keychain, int depth)
{
	sol::table item = t;
	for(int i = 0; i <= depth; i++) {
		sol::object o;
		if (std::holds_alternative<int>(keychain[i])) {
			o = item.raw_get<sol::object>(std::get<int>(keychain[i]));
		} else {
			o = item.raw_get<sol::object>(std::get<std::string>(keychain[i]));
		}
		if (o.get_type() == sol::type::table) {
			item = o.as<sol::table>();
		} else {
			return o;
		}
	}
	return item;
}

std::function<void(sol::object, const sol::object&, sol::object, sol::this_state)>
createNewIndexFunction(std::shared_ptr<Rml::SolLua::SolLuaDataModel> data, const std::vector<std::variant<int, std::string>>& keychain, int depth)
{
	return ([data, keychain, depth] (sol::table t, const sol::object& solkey, sol::object value, sol::this_state s) {
		auto prop = getFromTable(data->Table, keychain, depth-1);
		if (!prop.is<sol::table>()) {
			return;
		}
		auto old = prop.as<sol::table>().raw_get<sol::object>(solkey);
		if (old == value) {
			return;
		}
		if (value.is<sol::table>()) {
			auto value_raw = value.as<sol::table>().raw_get<sol::object>("__raw");
			if (value_raw != sol::lua_nil && value_raw.is<sol::function>()) {
				// new value is a datamodel proxy, so get the underlying table to assign
				prop.as<sol::table>().raw_set(solkey, value_raw.as<sol::function>().call<sol::object>(value));
			} else {
				prop.as<sol::table>().raw_set(solkey, value);
			}
		} else {
			prop.as<sol::table>().raw_set(solkey, value);
		}
		data->Handle.DirtyVariable(std::get<std::string>(keychain[0]));
	});
}

std::function<sol::object(sol::object, const sol::object&, sol::this_state)>
createIndexFunction(std::shared_ptr<Rml::SolLua::SolLuaDataModel> data, const std::vector<std::variant<int, std::string>>& keychain, int depth)
{
	return ([data, keychain, depth](sol::table t, const sol::object& solkey, sol::this_state s) {
		std::vector<std::variant<int, std::string>> kc{keychain};
		if (solkey.is<std::string>())
			kc.push_back(solkey.as<std::string>());
		else
			kc.push_back(solkey.as<int>());
		auto prop = getFromTable(data->Table, kc, depth);
		auto type = prop.get_type();
		if (type == sol::type::table) {
			sol::state_view bindings{s};
			sol::table obj_metatable = bindings.create_table();
			sol::table obj_table = bindings.create_table();
			obj_metatable[sol::meta_function::index] = createIndexFunction(data, kc, depth+1);
			obj_metatable[sol::meta_function::new_index] = createNewIndexFunction(data, kc, depth+1);
			obj_metatable[sol::meta_function::pairs] = createPairsFunction();
			obj_metatable[sol::meta_function::ipairs] = createPairsFunction(true);

			obj_table["__ipairs"] = obj_metatable[sol::meta_function::ipairs];
			obj_table[sol::meta_function::length] = [prop] () { return prop.as<sol::table>().size(); };
			obj_table["__raw"] = [prop] () { return prop; };
			obj_table[sol::metatable_key] = obj_metatable;
			return sol::make_object(s, obj_table);
		}
		return prop;
	});
};

/// <summary>
/// Opens a Lua data model.
/// </summary>
/// <param name="self">The context that called this function.</param>
/// <param name="name">The name of the data model.</param>
/// <param name="model">The table to bind as the data model.</param>
/// <param name="s">Lua state.</param>
/// <returns>A unique pointer to a Sol Lua Data Model.</returns>
sol::table openDataModel(Rml::Context& self, const Rml::String& name, sol::object model, sol::this_state s)
{
	sol::state_view bindings{s};

	// Only bind table.
	if (model.get_type() != sol::type::table) {
		return sol::lua_nil;
	}

	// Create data model.
	auto constructor = self.CreateDataModel(name);

	// Already created?  Get existing.
	if (!constructor) {
		constructor = self.GetDataModel(name);
		if (!constructor)
			return sol::lua_nil;
	}

	auto data = std::make_shared<SolLuaDataModel>(bindings);
	data->Constructor = constructor;
	data->Handle = constructor.GetModelHandle();
	data->ObjectDef = std::make_unique<SolLuaObjectDef>(data.get());
	data->Table = model.as<sol::table>();
	datamodel::bindTable(data.get(), data->Table);

	auto new_index_func =
		([data](sol::object t, const std::string& key, sol::object value, sol::this_state s) {
			auto iter = data->BindingMap.find(key);
			if (iter == data->BindingMap.end())
				luaL_error(s, "Assigning a new key ('%s') to the DataModel root is not allowed.", key.c_str());
			if (iter->second == SolLuaDataModel::BindingType::Function)
				luaL_error(s, "Changing the value of a key ('%s') bound to a Function in a DataModel is not allowed.", key.c_str());

			auto old = data->Table.raw_get<sol::object>(key);
			if (old == value && !value.is<sol::table>()) {
				return;
			}
			if (value.is<sol::table>()) {
				auto value_raw = value.as<sol::table>().raw_get<sol::object>("__raw");
				if (value_raw != sol::lua_nil && value_raw.is<sol::function>()) {
					// new value is a datamodel proxy, so get the underlying table to assign
					data->Table.raw_set(key, value_raw.as<sol::function>().call<sol::object>(value));
				} else {
					data->Table.raw_set(key, value);
				}
			} else {
				data->Table.raw_set(key, value);
			}
			data->Handle.DirtyVariable(key);
		});

	auto index_function =
		([data](sol::object t, const std::string& key, sol::this_state s) {
			auto item = data->Table.raw_get<sol::object>(key);
			auto type = item.get_type();
			if (type == sol::type::table) {
				sol::state_view bindings{s};
				std::vector<std::variant<int, std::string>> keychain{key};

				sol::table obj_metatable = bindings.create_table();
				sol::table obj_table = bindings.create_table();
				obj_metatable[sol::meta_function::new_index] = createNewIndexFunction(data, keychain, 1);
				obj_metatable[sol::meta_function::index] = createIndexFunction(data, keychain, 1);
				obj_metatable[sol::meta_function::pairs] = createPairsFunction();
				obj_metatable[sol::meta_function::ipairs] = createPairsFunction(true);

				obj_table["__ipairs"] = obj_metatable[sol::meta_function::ipairs];
				obj_table[sol::meta_function::length] = [item] () { return item.as<sol::table>().size(); };
				obj_table["__raw"] = [item] () { return item; };
				obj_table[sol::metatable_key] = obj_metatable;
				return sol::make_object(s, obj_table);
			}
			return item;
		});

	sol::table obj_metatable = bindings.create_table();
	sol::table obj_table = bindings.create_table();
	obj_metatable[sol::meta_function::new_index] = new_index_func;
	obj_metatable[sol::meta_function::index] = index_function;
	obj_metatable[sol::meta_function::pairs] = createPairsFunction();
	obj_metatable[sol::meta_function::ipairs] = createPairsFunction(true);

	obj_table["__ipairs"] = obj_metatable[sol::meta_function::ipairs];
	obj_table[sol::meta_function::length] = [&prop = data->Table] (sol::object o) { return prop.size(); };
	obj_table["__SetDirty"] = ([data](sol::object t, const std::string& key) {
			data->Handle.DirtyVariable(key);
		});
	obj_table["__GetTable"] = ([data](sol::object t) {
			return data->Table;
		});
	obj_table["__raw"] = ([data](sol::object t) {
			return data->Table;
		});

	obj_table[sol::metatable_key] = obj_metatable;

	return obj_table;
}
}  // namespace datamodel

namespace element
{
auto getElementAtPoint1(Rml::Context& self, Rml::Vector2f point)
{
	return self.GetElementAtPoint(point);
}

auto getElementAtPoint2(Rml::Context& self, Rml::Vector2f point, Rml::Element& ignore)
{
	return self.GetElementAtPoint(point, &ignore);
}
}  // namespace element

/// <summary>
/// Binds the Rml::Context class to Lua.
/// </summary>
/// <param name="bindings">The Lua object to bind into.</param>
void bind_context(sol::table& namespace_table, SolLuaPlugin* slp)
{
	// clang-format off

	/*** Holds documents and a data model.
	 *
	 * The Context class has no constructor; it must be instantiated through the CreateContext() function. It has the following functions and properties:
	 * @class RmlUi.Context
	 */

	namespace_table.new_usertype<Rml::Context>(
		"Context", sol::no_constructor,
		// M
		/***
		 * Adds the inline Lua script, script, as an event listener to the context. element_context is an optional Element; if it is not None, then the script will be executed as if it was bound to that element.
		 * @function RmlUi.Context:AddEventListener
		 * @param event string
		 * @param script RmlUi.Element
		 * @param element_context boolean
		 * @param in_capture_phase boolean
		 */
		"AddEventListener", &Rml::Context::AddEventListener,
		/***
		 * Creates a new document with the tag name of tag.
		 * @function RmlUi.Context:CreateDocument
		 * @param tag string
		 * @return RmlUi.Document
		 */
		"CreateDocument", [slp](Rml::Context& self) {
			auto doc = self.CreateDocument();
			slp->AddDocumentTracking(doc);
			return doc;
		},
		/***
		 * Attempts to load a document from the RML file found at document_path. If successful, the document will be returned with a reference count of one.
		 * @function RmlUi.Context:LoadDocument
		 * @param document_path string
		 * @return RmlUi.Document
		 */
		"LoadDocument", [slp](Rml::Context& self, const Rml::String& document, const sol::object& widget) {
			auto doc = self.LoadDocument(document);
			if (doc == nullptr) {
				return (SolLuaDocument*)nullptr;
			}
			slp->AddDocumentTracking(doc);
			auto env = dynamic_cast<SolLuaDocument*>(doc)->GetLuaEnvironment();
			env["widget"] = widget;
			return dynamic_cast<SolLuaDocument*>(doc);
		},
		/***
		 * @function RmlUi.Context:GetDocument
		 * @param name string
		 */
		"GetDocument", &document::getDocumentBypassString,
		/***
		 * Renders the context.
		 * @function RmlUi.Context:Render
		 * @return boolean
		 */
		"Render", [](Rml::Context& self) {
			RmlGui::BeginFrame();
			bool result = self.Render();
			RmlGui::PresentFrame();
			return result;
		},
		/***
		 * Closes all documents currently loaded with the context.
		 * @function RmlUi.Context:UnloadAllDocuments
		 */
		"UnloadAllDocuments", &Rml::Context::UnloadAllDocuments,
		/***
		 * Unloads a specific document within the context.
		 * @function RmlUi.Context:UnloadDocument
		 * @param document RmlUi.Document
		 */
		"UnloadDocument", &Rml::Context::UnloadDocument,
		/***
		 * Updates the context.
		 * @function RmlUi.Context:Update
		 * @return boolean
		 */
		"Update", &Rml::Context::Update,
		/***
		 * Create a new data model from a base table `model` and register to the context. The model table is copied.
		 * Note that `widget` does not actually have to be a widget; it can be any table. This table can be accessed in widgets like `<button class="mode-button" onclick="widget:SetCamMode()">Set Dolly Mode</button>`. Also note that your data model is inaccessible in `onx` properties.
		 * @function RmlUi.Context:OpenDataModel
		 * @generic T
		 * @param name string
		 * @param model T
		 * @param widget table
		 * @return RmlUi.SolLuaDataModel<T>
		 */
		"OpenDataModel", &datamodel::openDataModel,
		/***
		 * Removes a data model from the context.
		 * @function RmlUi.Context:RemoveDataModel
		 * @param name string
		 */
		"RemoveDataModel", &Rml::Context::RemoveDataModel,
		/***
		 * Processes a mouse move event.
		 * @function RmlUi.Context:ProcessMouseMove
		 * @param position RmlUi.Vector2f
		 * @return boolean
		 */
		"ProcessMouseMove", &Rml::Context::ProcessMouseMove,
		/***
		 * Processes a mouse button down event.
		 * @function RmlUi.Context:ProcessMouseButtonDown
		 * @param button RmlUi.MouseButton
		 * @param key_modifier_state integer
		 * @return boolean
		 */
		"ProcessMouseButtonDown", &Rml::Context::ProcessMouseButtonDown,
		/***
		 * Processes a mouse button up event.
		 * @function RmlUi.Context:ProcessMouseButtonUp
		 * @param button RmlUi.MouseButton
		 * @param key_modifier_state integer
		 * @return boolean
		 */
		"ProcessMouseButtonUp", &Rml::Context::ProcessMouseButtonUp,
		/***
		 * Processes a mouse wheel event.
		 * @function RmlUi.Context:ProcessMouseWheel
		 * @param delta RmlUi.Vector2f | number
		 * @param key_modifier_state integer
		 * @return boolean
		 */
		"ProcessMouseWheel", sol::overload(
			static_cast<bool (Rml::Context::*)(float, int)>(&Rml::Context::ProcessMouseWheel),
			static_cast<bool (Rml::Context::*)(Vector2f, int)>(&Rml::Context::ProcessMouseWheel)),
		/***
		 * Processes a mouse leave event.
		 * @function RmlUi.Context:ProcessMouseLeave
		 * @return boolean
		 */
		"ProcessMouseLeave", &Rml::Context::ProcessMouseLeave,
		/***
		 * Returns true if the mouse is currently interacting with the context, false if not.
		 * @function RmlUi.Context:IsMouseInteracting
		 * @return boolean
		 */
		"IsMouseInteracting", &Rml::Context::IsMouseInteracting,
		/***
		 * Processes a key down event.
		 * @function RmlUi.Context:ProcessKeyDown
		 * @param key RmlUi.key_identifier
		 * @param key_modifier_state integer
		 * @return boolean
		 */
		"ProcessKeyDown", &Rml::Context::ProcessKeyDown,
		/***
		 * Processes a key up event.
		 * @function RmlUi.Context:ProcessKeyUp
		 * @param key RmlUi.key_identifier
		 * @param key_modifier_state integer
		 * @return boolean
		 */
		"ProcessKeyUp", &Rml::Context::ProcessKeyUp,
		/***
		 * Processes a text input event.
		 * @function RmlUi.Context:ProcessTextInput
		 * @param text string
		 * @return boolean
		 */
		"ProcessTextInput", sol::resolve<bool(const Rml::String&)>(&Rml::Context::ProcessTextInput),
		//--
		/***
		 * Enables or disables the mouse cursor for the context.
		 * @function RmlUi.Context:EnableMouseCursor
		 * @param enable boolean
		 */
		"EnableMouseCursor", &Rml::Context::EnableMouseCursor,
		/***
		 * Activates a theme for the context.
		 * @function RmlUi.Context:ActivateTheme
		 * @param theme_name string
		 * @param activate boolean
		 */
		"ActivateTheme",&Rml::Context::ActivateTheme,
		/***
		 * Returns true if the theme is active, false if not.
		 * @function RmlUi.Context:IsThemeActive
		 * @param theme_name string
		 * @return boolean
		 */
		"IsThemeActive", &Rml::Context::IsThemeActive,
		/***
		 * Returns the element at the point specified by point.
		 * @function RmlUi.Context:GetElementAtPoint
		 * @param point RmlUi.Vector2f
		 * @param ignore RmlUi.Element?
		 * @return RmlUi.Element
		 */
		"GetElementAtPoint", sol::overload(&element::getElementAtPoint1, &element::getElementAtPoint2),
		/***
		 * Pulls the document to the front of the context.
		 * @function RmlUi.Context:PullDocumentToFront
		 * @param document RmlUi.Document
		 */
		"PullDocumentToFront", &Rml::Context::PullDocumentToFront,
		/***
		 * Pushes the document to the back of the context.
		 * @function RmlUi.Context:PushDocumentToBack
		 * @param document RmlUi.Document
		 */
		"PushDocumentToBack",&Rml::Context::PushDocumentToBack, "UnfocusDocument", &Rml::Context::UnfocusDocument,

		// G+S
		/*** @field RmlUi.Context.dimensions RmlUi.Vector2i */
		"dimensions", sol::property(&Rml::Context::GetDimensions, &Rml::Context::SetDimensions),
		/*** @field RmlUi.Context.dp_ratio number */
		"dp_ratio", sol::property(&Rml::Context::GetDensityIndependentPixelRatio, &Rml::Context::SetDensityIndependentPixelRatio),
		// G
		/*** @field RmlUi.Context.documents RmlUi.Document[] */
		"documents", sol::readonly_property(&getIndexedTable<SolLuaDocument, Rml::Context, &document::getDocument, &Rml::Context::GetNumDocuments>),
		/*** @field RmlUi.Context.focus_element RmlUi.Element */
		"focus_element", sol::readonly_property(&Rml::Context::GetFocusElement),
		/*** @field RmlUi.Context.hover_element RmlUi.Element */
		"hover_element", sol::readonly_property(&Rml::Context::GetHoverElement),
		/*** @field RmlUi.Context.name string */
		"name", sol::readonly_property(&Rml::Context::GetName),
		/*** @field RmlUi.Context.root_element RmlUi.Element */
		"root_element", sol::readonly_property(&Rml::Context::GetRootElement));
	// clang-format on
}

}  // end namespace Rml::SolLua
