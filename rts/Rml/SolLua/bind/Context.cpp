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

#include <memory>

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
		auto it = data->ObjectMap.insert_or_assign(skey, value);

		if (value.get_type() == sol::type::function) {
			data->Constructor.BindEventCallback(skey, [skey, cb = sol::protected_function{value},
			                                           state = sol::state_view{table.lua_state()}](
														  Rml::DataModelHandle, Rml::Event& event,
														  const Rml::VariantList& varlist) {
				if (cb.valid()) {
					std::vector<sol::object> args;
					for (const auto& variant : varlist) {
						args.push_back(makeObjectFromVariant(&variant, state));
					}
					auto pfr = cb(event, sol::as_args(args));
					if (!pfr.valid())
						ErrorHandler(cb.lua_state(), std::move(pfr));
				}
			});
			data->BindingMap[skey] = SolLuaDataModel::BindingType::Function;
		} else {
			data->Constructor.BindCustomDataVariable(
				skey, Rml::DataVariable(data->ObjectDef.get(), &(it.first->second)));
			data->BindingMap[skey] = SolLuaDataModel::BindingType::Variable;
		}
	}
}

/// <summary>
/// Opens a Lua data model.
/// </summary>
/// <param name="self">The context that called this function.</param>
/// <param name="name">The name of the data model.</param>
/// <param name="model">The table to bind as the data model.</param>
/// <param name="s">Lua state.</param>
/// <returns>A unique pointer to a Sol Lua Data Model.</returns>
sol::table openDataModel(Rml::Context& self, const Rml::String& name, sol::object model,
                         sol::this_state s)
{
	sol::state_view bindings{s};

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

	// Only bind table.
	if (model.get_type() == sol::type::table) {
		data->Table = model.as<sol::table>();
		datamodel::bindTable(data.get(), data->Table);
	}

	auto obj_table = bindings.create_table();
	auto new_index_func =  //
		([data](sol::object t, const std::string& key, sol::object value, sol::this_state s) {
			auto iter = data->BindingMap.find(key);
			if (iter == data->BindingMap.end())
				luaL_error(s, "Assigning a new key ('%s') in a DataModel is not allowed.",
			               key.c_str());

			if (iter->second == SolLuaDataModel::BindingType::Function)
				luaL_error(
					s,
					"Changing the value of a key ('%s') bound to a Function in a DataModel is not "
					"allowed.",
					key.c_str());

			data->Table.raw_set(key, value);
			data->ObjectMap.insert_or_assign(key, value);
			data->Handle.DirtyVariable(key);
		});

	sol::table obj_metatable = bindings.create_table();
	obj_metatable[sol::meta_function::new_index] = new_index_func;

	obj_metatable[sol::meta_function::index] =
		([data](sol::object t, const std::string& key, sol::this_state s) {
			return data->Table.get<sol::object>(key);
		});

	obj_table[sol::metatable_key] = obj_metatable;

	// absolutely no assigning of keys to the top level table allowed
	sol::table internal_data_metatable = bindings.create_table();
	internal_data_metatable[sol::meta_function::new_index] = new_index_func;
	data->Table[sol::metatable_key] = internal_data_metatable;

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
	namespace_table.new_usertype<Rml::Context>(
		"Context", sol::no_constructor,
		// M
		"AddEventListener", &Rml::Context::AddEventListener,
		"CreateDocument", [slp](Rml::Context& self) {
			auto doc = self.CreateDocument();
			slp->AddDocumentTracking(doc);
			return doc;
		},
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
		"GetDocument", &document::getDocumentBypassString,
		"Render", &Rml::Context::Render,
		"UnloadAllDocuments", &Rml::Context::UnloadAllDocuments,
		"UnloadDocument", &Rml::Context::UnloadDocument,
		"Update", &Rml::Context::Update,
		"OpenDataModel", &datamodel::openDataModel,
		"RemoveDataModel", &Rml::Context::RemoveDataModel,
		"ProcessMouseMove", &Rml::Context::ProcessMouseMove,
		"ProcessMouseButtonDown", &Rml::Context::ProcessMouseButtonDown,
		"ProcessMouseButtonUp", &Rml::Context::ProcessMouseButtonUp,
		"ProcessMouseWheel", sol::overload(
			static_cast<bool (Rml::Context::*)(float, int)>(&Rml::Context::ProcessMouseWheel),
			static_cast<bool (Rml::Context::*)(Vector2f, int)>(&Rml::Context::ProcessMouseWheel)),
		"ProcessMouseLeave", &Rml::Context::ProcessMouseLeave,
		"IsMouseInteracting", &Rml::Context::IsMouseInteracting,
		"ProcessKeyDown", &Rml::Context::ProcessKeyDown,
		"ProcessKeyUp", &Rml::Context::ProcessKeyUp,
		"ProcessTextInput", sol::resolve<bool(const Rml::String&)>(&Rml::Context::ProcessTextInput),
		//--
		"EnableMouseCursor", &Rml::Context::EnableMouseCursor,
		"ActivateTheme",&Rml::Context::ActivateTheme,
		"IsThemeActive", &Rml::Context::IsThemeActive,
		"GetElementAtPoint", sol::overload(&element::getElementAtPoint1, &element::getElementAtPoint2),
		"PullDocumentToFront", &Rml::Context::PullDocumentToFront,
		"PushDocumentToBack",&Rml::Context::PushDocumentToBack, "UnfocusDocument", &Rml::Context::UnfocusDocument,
		// RemoveEventListener

		// G+S
		"dimensions", sol::property(&Rml::Context::GetDimensions, &Rml::Context::SetDimensions),
		"dp_ratio", sol::property(&Rml::Context::GetDensityIndependentPixelRatio, &Rml::Context::SetDensityIndependentPixelRatio),
		//--
		"clip_region", sol::property(&Rml::Context::GetActiveClipRegion, &Rml::Context::SetActiveClipRegion),

		// G
		"documents", sol::readonly_property(&getIndexedTable<SolLuaDocument, Rml::Context, &document::getDocument, &Rml::Context::GetNumDocuments>),
		"focus_element", sol::readonly_property(&Rml::Context::GetFocusElement),
		"hover_element", sol::readonly_property(&Rml::Context::GetHoverElement),
		"name", sol::readonly_property(&Rml::Context::GetName),
		"root_element", sol::readonly_property(&Rml::Context::GetRootElement));
	// clang-format on
}

}  // end namespace Rml::SolLua
