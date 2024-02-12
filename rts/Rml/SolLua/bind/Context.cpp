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
	sol::state_view lua{s};

	// Create data model.
	auto constructor = self.CreateDataModel(name);

	// Already created?  Get existing.
	if (!constructor) {
		constructor = self.GetDataModel(name);
		if (!constructor)
			return sol::lua_nil;
	}

	auto data = std::make_shared<SolLuaDataModel>(lua);
	data->Constructor = constructor;
	data->Handle = constructor.GetModelHandle();
	data->ObjectDef = std::make_unique<SolLuaObjectDef>(data.get());

	// Only bind table.
	if (model.get_type() == sol::type::table) {
		data->Table = model.as<sol::table>();
		datamodel::bindTable(data.get(), data->Table);
	}

	auto obj_table = lua.create_table();
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

	sol::table obj_metatable = lua.create_table();
	obj_metatable[sol::meta_function::new_index] = new_index_func;

	obj_metatable[sol::meta_function::index] =
		([data](sol::object t, const std::string& key, sol::this_state s) {
			return data->Table.get<sol::object>(key);
		});

	obj_table[sol::metatable_key] = obj_metatable;

	sol::table obj_metatable_2 = lua.create_table();
	obj_metatable_2[sol::meta_function::new_index] = new_index_func;

	data->Table[sol::metatable_key] = obj_metatable_2;

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
/// <param name="lua">The Lua state to bind into.</param>
void bind_context(sol::state_view& lua, SolLuaPlugin* slp)
{
	// clang-format off
	lua.new_usertype<Rml::Context>(
		"Context", sol::no_constructor,
		// M
		"AddEventListener", &Rml::Context::AddEventListener,
		"CreateDocument", [slp](Rml::Context& self) {
			auto doc = self.CreateDocument();
			slp->AddDocumentTracking(doc);
			return doc;
		},
		"LoadDocument", [slp](Rml::Context& self, const Rml::String& document, sol::object w,
							  sol::this_environment e, sol::this_state s) {
			auto doc = self.LoadDocument(document);
			if (doc == nullptr) {
				return (SolLuaDocument*)nullptr;
			}
			slp->AddDocumentTracking(doc);
			auto env = dynamic_cast<SolLuaDocument*>(doc)->GetLuaEnvironment();
			// sol::environment envi(e);
		    // env[sol::metatable_key] = e.env;
		    // sol::environment env(state, sol::create, state.globals());
			env["widget"] = w;
			// env["__HandleError"] = envi["__HandleError"];

			// dynamic_cast<SolLuaDocument*>(doc)->m_environment = env;
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
		// "ProcessMouseWheel", &Rml::Context::ProcessMouseWheel,
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
