/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <vector>

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
private:
	enum DataType : uint8_t {
		FREE = 0,
		BUSY = 1
	};
public:
	CRectangleOverlapHandler()
		: sizeX{ 0 }
		, sizeY{ 0 }
	{}
	CRectangleOverlapHandler(size_t sizeX_, size_t sizeY_)
		: sizeX{ sizeX_ }
		, sizeY{ sizeY_ }
		, updateContainer(sizeX * sizeY, DataType::FREE)
	{}
public:
	void push_back(const SRectangle& rect);
	void pop_front_n(size_t n);

	void Process(size_t maxArea);

	auto empty() const { return rectanglesVec.empty(); }
	auto size()  const { return rectanglesVec.size();  }

	auto front() const { return rectanglesVec.front(); }

	auto begin() const { return rectanglesVec.begin(); }
    auto end()   const { return rectanglesVec.end();   }
private:
	size_t sizeX;
	size_t sizeY;

	std::vector<DataType> updateContainer;
	std::vector<SRectangle> rectanglesVec;
};