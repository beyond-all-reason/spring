#include "bind.h"

#include "plugin/SolLuaDocument.h"
#include "plugin/SolLuaDataModel.h"

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
			std::function<SolLuaDocument* (int)> result = [&self](int idx) -> auto { return getDocumentBypass(self, idx); };
			return result;
		}
	}

	namespace datamodel
	{
		/// <summary>
		/// Bind a sol::table into the data model.
		/// </summary>
		/// <param name="data">The data model container.</param>
		/// <param name="table">The table to bind.</param>
		void bindTable(SolLuaDataModel* data, sol::table& table)
		{
			for (auto& [key, value] : table)
			{
				auto skey = key.as<std::string>();
				auto it = data->ObjectList.insert_or_assign(skey, value);

				if (value.get_type() == sol::type::function)
				{
					data->Constructor.BindEventCallback(skey,
						[skey, cb = sol::protected_function{ value }, state = sol::state_view{ table.lua_state() }](Rml::DataModelHandle, Rml::Event& event, const Rml::VariantList& varlist)
						{
							if (cb.valid())
							{
								std::vector<sol::object> args;
								for (const auto& variant : varlist)
								{
									args.push_back(makeObjectFromVariant(&variant, state));
								}
								auto pfr = cb(event, sol::as_args(args));
								if (!pfr.valid())
									ErrorHandler(cb.lua_state(), std::move(pfr));
							}
						});
				}
				else
				{
					data->Constructor.BindCustomDataVariable(skey, Rml::DataVariable(data->ObjectDef.get(), &(it.first->second)));
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
		std::unique_ptr<SolLuaDataModel> openDataModel(Rml::Context& self, const Rml::String& name, sol::object model, sol::this_state s)
		{
			sol::state_view lua{ s };

			// Create data model.
			auto constructor = self.CreateDataModel(name);
			auto data = std::make_unique<SolLuaDataModel>(lua);

			// Already created?  Get existing.
			if (!constructor)
			{
				constructor = self.GetDataModel(name);
				if (!constructor)
					return data;
			}

			data->Constructor = constructor;
			data->Handle = constructor.GetModelHandle();
			data->ObjectDef = std::make_unique<SolLuaObjectDef>(data.get());

			// Only bind table.
			if (model.get_type() == sol::type::table)
			{
				data->Table = model.as<sol::table>();
				datamodel::bindTable(data.get(), data->Table);
			}

			return data;
		}
	}

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
	}

	/// <summary>
	/// Binds the Rml::Context class to Lua.
	/// </summary>
	/// <param name="lua">The Lua state to bind into.</param>
	void bind_context(sol::state_view& lua)
	{
		lua.new_usertype<Rml::Context>("Context", sol::no_constructor,
			// M
			"AddEventListener", &Rml::Context::AddEventListener,
			"CreateDocument", [](Rml::Context& self) { return self.CreateDocument(); },
			"LoadDocument", [](Rml::Context& self, const Rml::String& document) {
				auto doc = self.LoadDocument(document);
				return dynamic_cast<SolLuaDocument*>(doc);
			},
			"GetDocument", &document::getDocumentBypassString,
			"Render", &Rml::Context::Render,
			"UnloadAllDocuments", &Rml::Context::UnloadAllDocuments,
			"UnloadDocument", &Rml::Context::UnloadDocument,
			"Update", &Rml::Context::Update,
			"OpenDataModel", &datamodel::openDataModel,
			"ProcessMouseMove", &Rml::Context::ProcessMouseMove,
			"ProcessMouseButtonDown", &Rml::Context::ProcessMouseButtonDown,
			"ProcessMouseButtonUp", &Rml::Context::ProcessMouseButtonUp,
			// "ProcessMouseWheel", &Rml::Context::ProcessMouseWheel,
			"ProcessMouseLeave", &Rml::Context::ProcessMouseLeave,
			"IsMouseInteracting", &Rml::Context::IsMouseInteracting,
			"ProcessKeyDown", &Rml::Context::ProcessKeyDown,
			"ProcessKeyUp", &Rml::Context::ProcessKeyUp,
			"ProcessTextInput", sol::resolve<bool(const Rml::String&)>(&Rml::Context::ProcessTextInput),
			//--
			"EnableMouseCursor", &Rml::Context::EnableMouseCursor,
			"ActivateTheme", &Rml::Context::ActivateTheme,
			"IsThemeActive", &Rml::Context::IsThemeActive,
			"GetElementAtPoint", sol::overload(&element::getElementAtPoint1, &element::getElementAtPoint2),
			"PullDocumentToFront", &Rml::Context::PullDocumentToFront,
			"PushDocumentToBack", &Rml::Context::PushDocumentToBack,
			"UnfocusDocument", &Rml::Context::UnfocusDocument,
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
			"root_element", sol::readonly_property(&Rml::Context::GetRootElement)
		);
	}

} // end namespace Rml::SolLua
