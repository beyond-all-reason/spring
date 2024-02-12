#pragma once

#include <memory>
#include <unordered_map>

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/DataVariable.h>
#include <RmlUi/Core/Variant.h>
#include <RmlUi/Core/DataTypes.h>

#include <sol2/sol.hpp>


namespace Rml::SolLua
{
	class SolLuaObjectDef;

	struct SolLuaDataModel
	{
		SolLuaDataModel(sol::state_view s) : Lua{ s } {}

		Rml::DataModelConstructor Constructor;
		Rml::DataModelHandle Handle;
		sol::state_view Lua;
		std::unique_ptr<SolLuaObjectDef> ObjectDef;

		// sol data types are reference counted.  Hold onto them as we use them.
		sol::table Table;

		enum class BindingType { None = 0, Variable = 1, Function = 2 };
		std::unordered_map<std::string, BindingType> BindingMap;
		std::unordered_map<std::string, sol::object> ObjectMap;
	};

	class SolLuaObjectDef final : public Rml::VariableDefinition
	{
	public:
		SolLuaObjectDef(SolLuaDataModel* model);
		bool Get(void* ptr, Rml::Variant& variant) override;
		bool Set(void* ptr, const Rml::Variant& variant) override;
		int Size(void* ptr) override;
		DataVariable Child(void* ptr, const Rml::DataAddressEntry& address) override;
	protected:
		SolLuaDataModel* m_model;
	};

} // end namespace Rml::SolLua
