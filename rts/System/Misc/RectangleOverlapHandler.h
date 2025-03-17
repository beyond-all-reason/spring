/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <vector>
#include <tuple>
#include <memory>
#include <shared_mutex>

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
		, statsInputRects{ 0 }
		, statsOutputRects{ 0 }
		, statsInputArea{ 0 }
		, statsOutputArea{ 0 }
		, mutex{ nullptr }
	{}
	CRectangleOverlapHandler(size_t sizeX_, size_t sizeY_)
		: sizeX{ sizeX_ }
		, sizeY{ sizeY_ }
		, statsInputRects{ 0 }
		, statsOutputRects{ 0 }
		, statsInputArea{ 0 }
		, statsOutputArea{ 0 }
		, updateContainer(sizeX * sizeY, DataType::FREE)
		, mutex{ std::make_unique<std::shared_mutex>() }
	{}
	~CRectangleOverlapHandler();

	CRectangleOverlapHandler(const CRectangleOverlapHandler&) = delete;
	CRectangleOverlapHandler(CRectangleOverlapHandler&&) noexcept = default;

	CRectangleOverlapHandler& operator=(const CRectangleOverlapHandler&) = delete;
	CRectangleOverlapHandler& operator=(CRectangleOverlapHandler&&) noexcept = default;
public:
	void push_back(const SRectangle& rect);
	void pop_front_n(size_t n);

	void Process();

	auto empty() const { return rectanglesVec.empty(); }
	auto size()  const { return rectanglesVec.size();  }

	auto front() const { return rectanglesVec.front(); }

	auto begin() const { return rectanglesVec.begin(); }
	auto end()   const { return rectanglesVec.end();   }

	const auto& operator[](size_t idx) const { return rectanglesVec[idx]; }
private:
	static inline const auto BusyPred = [](auto val) { return (val == DataType::BUSY); };

	std::vector<std::pair<SRectangle, int>> GetBoundingRectsData() const;
	SRectangle GetMaximalRectangle(const SRectangle& bRect);
	SRectangle GetGreedyRectangle(const SRectangle& bRect);
	SRectangle GetLineRectangle(const SRectangle& bRect);
	void ClearUpdateContainer(const SRectangle& rect);
private:
	size_t sizeX;
	size_t sizeY;

	size_t statsInputRects;
	size_t statsOutputRects;
	size_t statsInputArea;
	size_t statsOutputArea;

	std::vector<DataType> updateContainer;
	std::vector<SRectangle> rectanglesVec;

	std::unique_ptr<std::shared_mutex> mutex;
};