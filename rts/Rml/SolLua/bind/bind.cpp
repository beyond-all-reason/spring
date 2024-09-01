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

#include <functional>


namespace Rml::SolLua
{

	sol::object makeObjectFromVariant(const Rml::Variant* variant, sol::state_view s)
	{
		if (!variant) return sol::make_object(s, sol::nil);

		switch (variant->GetType())
		{
		case Rml::Variant::BOOL:
			return sol::make_object(s, variant->Get<bool>());
		case Rml::Variant::BYTE:
		case Rml::Variant::CHAR:
		case Rml::Variant::INT:
			return sol::make_object(s, variant->Get<int>());
		case Rml::Variant::INT64:
			return sol::make_object(s, variant->Get<int64_t>());
		case Rml::Variant::UINT:
			return sol::make_object(s, variant->Get<unsigned int>());
		case Rml::Variant::UINT64:
			return sol::make_object(s, variant->Get<uint64_t>());
		case Rml::Variant::FLOAT:
		case Rml::Variant::DOUBLE:
			return sol::make_object(s, variant->Get<lua_Number>());
		case Rml::Variant::COLOURB:
			return sol::make_object_userdata<Rml::Colourb>(s, variant->Get<Rml::Colourb>());
		case Rml::Variant::COLOURF:
			return sol::make_object_userdata<Rml::Colourf>(s, variant->Get<Rml::Colourf>());
		case Rml::Variant::STRING:
			return sol::make_object(s, variant->GetReference<Rml::String>());
		case Rml::Variant::VECTOR2:
			return sol::make_object_userdata<Rml::Vector2f>(s, variant->Get<Rml::Vector2f>());
		case Rml::Variant::VOIDPTR:
			return sol::make_object(s, variant->Get<void*>());
		default:
			return sol::make_object(s, sol::nil);
		}

		return sol::make_object(s, sol::nil);
	}

} // end namespace Rml::SolLua
