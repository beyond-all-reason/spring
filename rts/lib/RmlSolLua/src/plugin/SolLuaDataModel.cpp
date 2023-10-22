#include "SolLuaDataModel.h"

#include <optional>


namespace Rml::SolLua
{

	SolLuaObjectDef::SolLuaObjectDef(SolLuaDataModel* model)
		: VariableDefinition(DataVariableType::Scalar), m_model(model)
	{
	}

	bool SolLuaObjectDef::Get(void* ptr, Rml::Variant& variant)
	{
		auto obj = static_cast<sol::object*>(ptr);

		if (obj->is<bool>())
			variant = obj->as<bool>();
		else if (obj->is<std::string>())
			variant = obj->as<std::string>();
		else if (obj->is<Rml::Vector2i>())
			variant = obj->as<Vector2i>();
		else if (obj->is<Rml::Vector2f>())
			variant = obj->as<Vector2f>();
		else if (obj->is<Rml::Colourb>())
			variant = obj->as<Rml::Colourb>();
		else if (obj->is<Rml::Colourf>())
			variant = obj->as<Rml::Colourf>();
		else if (obj->is<double>())
			variant = obj->as<double>();
		else // if (obj->get_type() == sol::type::lua_nil)
			variant = Rml::Variant{};

		return true;
	}

	bool SolLuaObjectDef::Set(void* ptr, const Rml::Variant& variant)
	{
		auto obj = static_cast<sol::object*>(ptr);

		if (obj->is<bool>())
			variant.GetInto<bool>(*static_cast<bool*>(ptr));
		else if (obj->is<std::string>())
			variant.GetInto<std::string>(*static_cast<std::string*>(ptr));
		else if (obj->is<Rml::Vector2i>())
			variant.GetInto<Rml::Vector2i>(*static_cast<Rml::Vector2i*>(ptr));
		else if (obj->is<Rml::Vector2f>())
			variant.GetInto<Rml::Vector2f>(*static_cast<Rml::Vector2f*>(ptr));
		else if (obj->is<Rml::Colourb>())
			variant.GetInto<Rml::Colourb>(*static_cast<Rml::Colourb*>(ptr));
		else if (obj->is<Rml::Colourf>())
			variant.GetInto<Rml::Colourf>(*static_cast<Rml::Colourf*>(ptr));
		else if (obj->is<double>())
			variant.GetInto<double>(*static_cast<double*>(ptr));
		else // if (obj->get_type() == sol::type::lua_nil)
			*obj = sol::make_object(m_model->Lua, sol::nil);

		return true;
	}

	int SolLuaObjectDef::Size(void* ptr)
	{
		// Non-table types are 1 entry long.
		auto object = static_cast<sol::object*>(ptr);
		if (object->get_type() != sol::type::table)
			return 1;

		auto t = object->as<sol::table>();
		return static_cast<int>(t.size());
	}

	DataVariable SolLuaObjectDef::Child(void* ptr, const Rml::DataAddressEntry& address)
	{
		// Child should be called on a table.
		auto object = static_cast<sol::object*>(ptr);
		if (object->get_type() != sol::type::table)
			return DataVariable{};

		// Get our table object.
		// Get the pointer as a string for use with holding onto the object.
		sol::table table = object->as<sol::table>();
		std::string tablestr = std::to_string(reinterpret_cast<intptr_t>(table.pointer()));

		// Accessing by name.
		if (address.index == -1)
		{
			// Try to get the object.
			auto e = table.get<sol::object>(address.name);
			if (e.get_type() == sol::type::lua_nil)
				return DataVariable{};

			// Hold a reference to it and return the pointer.
			auto it = m_model->ObjectList.insert_or_assign(tablestr + "_" + address.name, e);
			return DataVariable{ m_model->ObjectDef.get(), &(it.first->second) };
		}
		// Accessing by index.
		else
		{
			// See if we have a key with the index.
			auto has_index = table.get<sol::object>(address.index+1);
			if (has_index.get_type() != sol::type::lua_nil)
			{
				auto it = m_model->ObjectList.insert_or_assign(tablestr + "_" + std::to_string(address.index+1), has_index);
				return DataVariable{ m_model->ObjectDef.get(), &(it.first->second) };
			}

			// Iterate through the entries and grab the nth entry.
			int idx = 1;
			for (auto& [k, v] : table.pairs())
			{
				if (idx == address.index+1)
				{
					auto it = m_model->ObjectList.insert_or_assign(tablestr + "_" + std::to_string(idx), v);
					return DataVariable{ m_model->ObjectDef.get(), &(it.first->second) };
				}
				++idx;
			}

			// Index out of range.
			return DataVariable{};
		}

		// Failure.
		return DataVariable{};
	}

} // end namespace Rml::SolLua
