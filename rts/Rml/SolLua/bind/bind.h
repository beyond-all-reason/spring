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

#include <RmlUi/Core.h>
#include <sol2/sol.hpp>

#include <type_traits>

#include "../plugin/SolLuaPlugin.h"


#ifndef RMLUI_NO_THIRDPARTY_CONTAINERS
template <typename Key, typename Value>
struct sol::is_container<Rml::UnorderedMap<Key, Value>> : std::true_type {};

template <typename Key, typename Value>
struct sol::is_container<Rml::SmallUnorderedMap<Key, Value>> : std::true_type {};

template <typename T>
struct sol::is_container<Rml::UnorderedSet<T>> : std::true_type {};

template <typename T>
struct sol::is_container<Rml::SmallUnorderedSet<T>> : std::true_type {};

//template <typename T>
//struct sol::is_container<Rml::SmallOrderedSet<T>> : std::true_type {};

template <>
struct sol::is_container<Rml::ElementList> : std::true_type {};
#endif


namespace Rml::SolLua
{

	sol::object makeObjectFromVariant(const Rml::Variant* variant, sol::state_view s);
	using SolObjectMap = std::unordered_map<std::string, sol::object>;

} // end namespace Rml::SolLua


namespace Rml::SolLua
{

	/// <summary>
	/// Constructs an iterator that can be used with sol::as_table() to return an integer indexed table.
	/// </summary>
	/// <typeparam name="T">The type of Rml::Element we are using for iteration.</typeparam>
	template <typename T = Rml::Element>
	struct TableIndexedIterator
	{
		struct Iter
		{
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = T*;
			using pointer = T**;
			using reference = T*&;

			Iter(const TableIndexedIterator<T>* owner, int pos)
				: m_owner{ owner }, m_pos{ pos }
			{}

			auto operator++() const
			{
				m_pos = std::min(++m_pos, m_owner->m_func_max());
				return *this;
			}

			auto operator++(int) const
			{
				auto tmp = *this;
				++(*this);
				return tmp;
			}

			auto operator==(const Iter& other) const
			{
				auto max = m_owner->m_func_max();
				auto my_pos = std::min(m_pos, max);
				auto other_pos = std::min(other.m_pos, max);
				return m_owner == other.m_owner && my_pos == other_pos;
			}
			auto operator!=(const Iter& other) const
			{
				return !(*this == other);
			}

			auto operator*() const
			{
				return m_owner->m_func_get(m_pos);
			}

		private:
			const TableIndexedIterator<T>* m_owner;
			mutable int m_pos = 0;
		};

		/// <summary>
		/// Constructs the iterator.
		/// </summary>
		/// <param name="element">The element we are pulling data from.</param>
		/// <param name="get">The function to get new data (ex: Rml::Element::GetChild). Must be of type std::function<T* (int)>.</param>
		/// <param name="max">The function to get the max number of items (ex: Rml::Element::GetNumChildren). Must be of type std::function<int ()>.</param>
		TableIndexedIterator(std::function<T* (int)> get, std::function<int()> max)
			: m_func_get{ get }, m_func_max{ max }
		{
			assert((m_func_get) && (m_func_max));
		}

		Iter begin() const
		{
			Iter it(this, 0);
			return it;
		}

		Iter end() const
		{
			Iter it(this, m_func_max());
			return it;
		}

		int size() const
		{
			return m_func_max();
		}

		friend Iter;

	private:
		std::function<T* (int)> m_func_get;
		std::function<int ()> m_func_max;
	};

	template <typename T, typename S, auto G, auto M>
	sol::as_table_t<TableIndexedIterator<T>> getIndexedTable(S& self)
	{
		std::function<T* (int)> get;
		if constexpr (std::is_member_function_pointer_v<decltype(G)>)
		{
			// Straight member function pointer like Rml::Element::GetChild.
			get = std::bind(G, &self, std::placeholders::_1);
		}
		else
		{
			// A helper function to convert the normal getter to type std::function<T* (int)>.
			auto f = std::invoke(G, self);
			get = f;
		}

		std::function<int()> max;
		if constexpr (std::is_member_function_pointer_v<decltype(M)>)
		{
			// Straight member function pointer like Rml::Element::GetNumChildren.
			max = std::bind(M, &self);
		}
		else
		{
			// A helper function to convert the normal getter to type std::function<int ())>.
			auto f = std::invoke(M, self);
			max = f;
		}

		TableIndexedIterator<T> result{ get, max };
		return sol::as_table(result);
	}

	template <typename T, auto G, auto M>
	sol::as_table_t<TableIndexedIterator<T>> getIndexedTable()
	{
		std::function<T* (int)> get;
		std::function<int()> max;

		// A helper function to convert the normal getter to type std::function<T* (int)>.
		auto fget = std::invoke(G);
		get = fget;

		// A helper function to convert the normal getter to type std::function<int ())>.
		auto fmax = std::invoke(M);
		max = fmax;

		TableIndexedIterator<T> result{ get, max };
		return sol::as_table(result);
	}

} // end namespace Rml::SolLua


namespace Rml::SolLua
{

	// Called from RmlSolLua.cpp
	void bind_color(sol::table& namespace_table);
	void bind_context(sol::table& namespace_table, SolLuaPlugin *slp);
	void bind_datamodel(sol::table& namespace_table);
	void bind_document(sol::table& namespace_table);
	void bind_element(sol::table& namespace_table);
	void bind_element_derived(sol::table& namespace_table);
	void bind_element_form(sol::table& namespace_table);
	void bind_event(sol::table& namespace_table);
	void bind_global(sol::table& namespace_table, SolLuaPlugin *slp);
	void bind_vector(sol::table& namespace_table);
	void bind_convert(sol::table& namespace_table);

} // end namespace Rml::SolLua
