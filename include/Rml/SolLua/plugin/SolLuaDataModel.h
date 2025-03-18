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
