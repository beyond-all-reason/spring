/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <vector>
#include <tuple>
#include <iterator>

#include "System/Rectangle.h"
#include "System/creg/creg_cond.h"


/**
 * @brief CRectangleOverlapHandler
 *
 * Container & preprocessor for rectangles. It handles any overlap & merges+resizes rectangles.
 */
class CRectangleOverlapHandler
{
	CR_DECLARE_STRUCT(CRectangleOverlapHandler)

public:
	CRectangleOverlapHandler()
		: sizeX{ 0 }
		, sizeY{ 0 }
		, maxSideSize{ 0 }
		, updateCounter{ 0 }
	{}
	CRectangleOverlapHandler(size_t sizeX_, size_t sizeY_, int maxSideSize_)
		: sizeX{ sizeX_ }
		, sizeY{ sizeY_ }
		, maxSideSize{ maxSideSize_ }
		, updateCounter{ 0 }
		, updateContainer(sizeX * sizeY, EMPTY)
	{}
public:
	using RectanglesVecType = std::vector<std::pair<uint64_t, SRectangle>>;
	class const_iterator {
	public:
		// Iterator traits required by C++ standard
		using iterator_category = std::random_access_iterator_tag;
		using value_type = SRectangle;
		using difference_type = std::ptrdiff_t;
		using pointer = const SRectangle*;
		using reference = const SRectangle&;

		explicit const_iterator(RectanglesVecType::const_iterator it)
			: m_it(it)
		{}

		// Dereference operator returns reference to SRectangle
		reference operator*() const { return m_it->second; }
		pointer operator->() const { return &m_it->second; }

		// Arithmetic operators
		const_iterator& operator++() { ++m_it; return *this; }
		const_iterator operator++(int) { auto tmp = *this; ++m_it; return tmp; }
		const_iterator& operator--() { --m_it; return *this; }
		const_iterator operator--(int) { auto tmp = *this; --m_it; return tmp; }
		const_iterator operator+(difference_type n) const { return const_iterator(m_it + n); }

		// Comparison operators
		bool operator==(const const_iterator& other) const { return m_it == other.m_it; }
		bool operator!=(const const_iterator& other) const { return m_it != other.m_it; }

	private:
		std::vector<std::pair<uint64_t, SRectangle>>::const_iterator m_it;
	};
public:
	void push_back(const SRectangle& rect, bool noSplit = false);
	void pop_front_n(size_t n);

	auto empty() const { return rectanglesVec.empty(); }
	auto size()  const { return rectanglesVec.size();  }

	decltype(auto) front() const { return rectanglesVec.begin()->second; }

	auto begin() const { return const_iterator(rectanglesVec.begin()); }
    auto end()   const { return const_iterator(rectanglesVec.end());   }
private:
	size_t sizeX;
	size_t sizeY;

	int maxSideSize;

	uint64_t updateCounter;

	std::vector<uint64_t> updateContainer;
	RectanglesVecType rectanglesVec;

	static constexpr uint64_t EMPTY = uint64_t(~0);
};