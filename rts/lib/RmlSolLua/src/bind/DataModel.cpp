#include "bind.h"

#include "plugin/SolLuaDocument.h"
#include "plugin/SolLuaDataModel.h"


namespace Rml::SolLua
{
	namespace functions
	{
		sol::object dataModelGet(SolLuaDataModel& self, const std::string& name, sol::this_state s)
		{
			return self.Table.get<sol::object>(name);
		}

		void dataModelSet(SolLuaDataModel& self, const std::string& name, sol::object value, sol::this_state s)
		{
			self.Table.set(name, value);
			self.Handle.DirtyVariable(name);
		}

		void dataModelSetDirty(SolLuaDataModel& self, const std::string& name)
		{
			self.Handle.DirtyVariable(name);
		}
	}

	void bind_datamodel(sol::state_view& lua)
	{

		lua.new_usertype<SolLuaDataModel>("SolLuaDataModel", sol::no_constructor,
			sol::meta_function::index, &functions::dataModelGet,
			sol::meta_function::new_index, &functions::dataModelSet,
			"__SetDirty", &functions::dataModelSetDirty
		);

	}

} // end namespace Rml::SolLua
