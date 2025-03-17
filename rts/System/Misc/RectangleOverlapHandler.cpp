/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RectangleOverlapHandler.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Pair.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

//#define ROH_MAKE_BITMAP_SNAPSHOTS
#ifdef ROH_MAKE_BITMAP_SNAPSHOTS
#include "Rendering/Textures/Bitmap.h"
#include <fmt/format.h>
#endif

#include <cassert>
#include <array>
#include <cmath>
#include <list>

CR_BIND(CRectangleOverlapHandler, )
CR_REG_METADATA(CRectangleOverlapHandler, (
	CR_MEMBER(sizeX),
	CR_MEMBER(sizeY),
	CR_MEMBER(statsInputRects),
	CR_MEMBER(statsOutputRects),
	CR_MEMBER(statsInputArea),
	CR_MEMBER(statsOutputArea),
	CR_MEMBER(updateContainer),
	CR_MEMBER(rectanglesVec)
))

CRectangleOverlapHandler::~CRectangleOverlapHandler()
{
	if (statsInputRects == 0 || statsInputArea == 0)
		return;

	const float countReduction = (100.0f * (statsInputRects - statsOutputRects)) / statsInputRects;
	const float areaReduction = (100.0f * (statsInputArea - statsOutputArea)) / statsInputArea;

	LOG("[%s] count / area reduction efficiency (%.0f%% / %.0f%%)", __func__, countReduction, areaReduction);
}

void CRectangleOverlapHandler::push_back(const SRectangle& rect)
{
	if (updateContainer.empty())
		return;

	if (rect.GetArea() <= 0)
		return;

	rectanglesVec.emplace_back(rect);
}

void CRectangleOverlapHandler::pop_front_n(size_t n)
{
	if (rectanglesVec.empty())
		return;

	decltype(rectanglesVec) tmpMap;
	tmpMap.assign(rectanglesVec.begin() + n, rectanglesVec.end());
	rectanglesVec = std::move(tmpMap);
}

void CRectangleOverlapHandler::Process()
{
	SCOPED_TIMER("CRectangleOverlapHandler::Process");

	if (rectanglesVec.empty())
		return;

	for (const auto& rect : rectanglesVec) {
		for (int y = rect.y1; y <= rect.y2; ++y) {
			auto off = updateContainer.begin() + y * sizeX;
			auto beg = off + rect.x1 + 0;
			auto end = off + rect.x2 + 1;
			std::fill(beg, end, DataType::BUSY);
		}
		statsInputArea += rect.GetArea();
	}
	statsInputRects += rectanglesVec.size();
	rectanglesVec.clear();


#ifdef ROH_MAKE_BITMAP_SNAPSHOTS
	size_t step = 0;
	CBitmap bitmap(nullptr, sizeX, sizeY, 1);
	auto* mem = bitmap.GetRawMem();
	for (auto value : updateContainer) {
		*mem = static_cast<uint8_t>(value == DataType::BUSY) * 0xFF;
		++mem;
	}
	bitmap.Save(fmt::format("CRectangleOverlapHandler-{}.png", step++), true);
#endif

	std::array<decltype(rectanglesVec), ThreadPool::MAX_THREADS> perThreadRectangles;

	auto boundingRectsData = GetBoundingRectsData();

	const auto MainLoopBody = [&boundingRectsData, &perThreadRectangles, this](int bri) {
		auto& [bRect, occArea] = boundingRectsData[bri];
		const auto bRectArea = bRect.GetArea();

		enum RectDecompositionAlgo {
			MAXIMAL,
			GREEDY,
			LINE
		};
		RectDecompositionAlgo algo = MAXIMAL;

		static constexpr auto MAXIMAL_ALGO_AREA_THRESHOLD = 0.25f;
		static constexpr auto GREEDY_ALGO_SPARSITY_THRESHOLD = 0.05f;

		while (true) {
			SRectangle rect;

			switch (algo)
			{
			case MAXIMAL:
				rect = GetMaximalRectangle(bRect);
				break;
			case GREEDY:
				rect = GetGreedyRectangle(bRect);
				break;
			case LINE:
				rect = GetLineRectangle(bRect);
				break;
			default:
				assert(false);
				break;
			}

			const auto area = rect.GetArea();
			if (area <= 0)
				break;

			assert(bRect.Inside(rect));

			occArea -= area;

			if (algo == RectDecompositionAlgo::MAXIMAL && static_cast<float>(area) / bRectArea < MAXIMAL_ALGO_AREA_THRESHOLD) {
				algo = RectDecompositionAlgo::GREEDY;
			}
			else if (algo == RectDecompositionAlgo::GREEDY && static_cast<float>(occArea) / bRectArea < GREEDY_ALGO_SPARSITY_THRESHOLD) {
				algo = RectDecompositionAlgo::LINE;
			}

			ClearUpdateContainer(rect);

			// make inclusive
			//rectanglesVec.emplace_back(rect.x1, rect.y1, rect.x2 - 1, rect.y2 - 1);
			perThreadRectangles[ThreadPool::GetThreadNum()].emplace_back(rect.x1, rect.y1, rect.x2 - 1, rect.y2 - 1);
#ifdef ROH_MAKE_BITMAP_SNAPSHOTS
			mem = bitmap.GetRawMem();
			for (auto value : updateContainer) {
				*mem = static_cast<uint8_t>(value == DataType::BUSY) * 0xFF;
				++mem;
			}
			bitmap.Save(fmt::format("CRectangleOverlapHandler-{}.png", step++), true);
#endif
		}
	};

#ifndef ROH_MAKE_BITMAP_SNAPSHOTS
	for_mt(0, static_cast<int>(boundingRectsData.size()), MainLoopBody);
#else
	for (int bri = 0; bri < static_cast<int>(boundingRectsData.size()); ++bri)
		MainLoopBody(bri);
#endif

	assert(rectanglesVec.empty());

	for (const auto& thisThreadRectangles : perThreadRectangles) {
		for (const auto& rect : thisThreadRectangles) {
			statsOutputArea += rect.GetArea();
			rectanglesVec.emplace_back(rect);
		}
	}

	statsOutputRects += rectanglesVec.size();
}

std::vector<std::pair<SRectangle, int>> CRectangleOverlapHandler::GetBoundingRectsData() const
{
	ZoneScoped;

	std::vector<std::pair<SRectangle, int>> boundingRectsData;

	struct Line {
		int y;
		int x1;
		int x2;
	};

	std::list<Line> lines;

	for (uint32_t y = 0; y < sizeY; y++) {
		const auto stt = updateContainer.begin() + y * sizeX;
		const auto fin = stt + sizeX;
		auto beg = stt;
		auto end = fin;
		while (beg != fin) {
			beg = std::find_if    (beg, fin, BusyPred);
			end = std::find_if_not(beg, fin, BusyPred);
			if (beg != end)
				lines.emplace_back(y, static_cast<int>(std::distance(stt, beg)), static_cast<int>(std::distance(stt, end)));

			beg = end; //rewind
		}
	}

	while (!lines.empty()) {
		auto beg = lines.begin();

		int busyArea = 0;
		SRectangle bRect {
			beg->x1,
			beg->y,
			beg->x2,
			beg->y + 1
		};
		busyArea += beg->x2 - beg->x1;

		// pop the previous item
		beg = lines.erase(beg);

		for (auto it = beg; it != lines.end(); /*NOOP*/) {
			if (std::abs(it->y - bRect.y2) > 1) {
				break;
			}
			busyArea += it->x2 - it->x1;

			const bool misalignedByX = (it->x2 < bRect.x1 || it->x1 > bRect.x2);
			if (misalignedByX) {
				++it;
				continue;
			}

			bRect.y2 = it->y + 1;
			bRect.x1 = std::min(bRect.x1, it->x1);
			bRect.x2 = std::max(bRect.x2, it->x2);

			it = lines.erase(it); // advances it
		}

		boundingRectsData.emplace_back(bRect, busyArea);
	}

	return boundingRectsData;
}

SRectangle CRectangleOverlapHandler::GetMaximalRectangle(const SRectangle& bRect)
{
	ZoneScoped;

	std::shared_lock lock(*mutex); //shared for read;

	std::vector<int> heights(sizeX + 1, 0); // Include extra element for easier calculation
	std::vector<int> stack;
	stack.reserve(sizeX);

	SRectangle rect{
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::min(),
		std::numeric_limits<int16_t>::min()
	};

	int maxArea = 0;

	for (int y = bRect.y1; y < bRect.y2; y++) {
		for (int x = bRect.x1; x < bRect.x2; x++) {
			auto itVal = updateContainer.begin() + y * sizeX + x;
			heights[x] = (*itVal == DataType::BUSY) ? heights[x] + 1 : 0;
		}

		stack.push_back(std::max(bRect.x1 - 1, 0));
		// Calculate max area using stack-based method
		for (int x = bRect.x1; x <= bRect.x2; x++) {
			while (!stack.empty() && heights[x] < heights[stack.back()]) {
				const int h = heights[stack.back()];
				stack.pop_back();
				const int w = stack.empty() ? x : x - stack.back() - 1;
				const int area = h * w;
				if (area > maxArea) {
					maxArea = area;
					rect.x1 = stack.empty() ? 0 : stack.back() + 1;
					rect.x2 = x - 1;
					rect.y2 = y;
					rect.y1 = y - h + 1;
				}
			}
			stack.push_back(x);
		}

		stack.clear();
	}

	if (maxArea == 0)
		return SRectangle();

	// make non-inclusive
	++rect.x2;
	++rect.y2;

	return rect;
}

SRectangle CRectangleOverlapHandler::GetGreedyRectangle(const SRectangle& bRect)
{
	ZoneScoped;

	std::shared_lock lock(*mutex); //shared for read;

	SRectangle rect {
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::min(),
		std::numeric_limits<int16_t>::min()
	};

	// find the first row
	for (int y = bRect.y1; y < bRect.y2; y++) {
		const auto off = updateContainer.begin() + y * sizeX;
		const auto stt = off + bRect.x1;
		const auto fin = off + bRect.x2;
		auto beg = stt;
		auto end = fin;

		beg = std::find_if    (beg, end, BusyPred);
		if (beg == fin)
			continue;

		end = std::find_if_not(beg, end, BusyPred);

		rect.x1 = static_cast<int>(std::distance(off, beg));
		rect.x2 = static_cast<int>(std::distance(off, end));
		rect.y1 = y;
		rect.y2 = y + 1;

		break;
	}

	if (rect.y2 < 0)
		return rect;

	for (int y = rect.y2; y < bRect.y2; y++) {
		const auto off = updateContainer.begin() + y * sizeX;
		const auto stt = off + rect.x1;
		const auto fin = off + rect.x2;
		auto beg = stt;
		auto end = fin;

		beg = std::find_if    (beg, end, BusyPred);
		end = std::find_if_not(beg, end, BusyPred);

		int x1 = static_cast<int>(std::distance(off, beg));
		int x2 = static_cast<int>(std::distance(off, end));

		if (x1 > rect.x1 || x2 < rect.x2)
			return rect;

		rect.y2 = y + 1;
	}

	return rect;
}

SRectangle CRectangleOverlapHandler::GetLineRectangle(const SRectangle& bRect)
{
	ZoneScoped;

	std::shared_lock lock(*mutex); //shared for read;

	SRectangle rect {
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::min(),
		std::numeric_limits<int16_t>::min()
	};

#if 1
	// find the topmost row, leftmost contiguous column
	for (int y = bRect.y1; y < bRect.y2; y++) {
		const auto off = updateContainer.begin() + y * sizeX;
		const auto stt = off + bRect.x1;
		const auto fin = off + bRect.x2;
		auto beg = stt;
		auto end = fin;

		beg = std::find_if(beg, end, BusyPred);
		if (beg == fin)
			continue;

		end = std::find_if_not(beg, end, BusyPred);

		rect.x1 = static_cast<int>(std::distance(off, beg));
		rect.x2 = static_cast<int>(std::distance(off, end));
		rect.y1 = y;
		rect.y2 = y + 1;

		break;
	}
#else
	// find the first busy point
	for (int y = bRect.y1; y < bRect.y2; y++) {
		for (int x = bRect.x1; x < bRect.x2; x++) {
			size_t idx = y * sizeX + x;
			if (BusyPred(updateContainer[idx])) {
				rect.x1 = x;
				rect.x2 = x + 1;
				rect.y1 = y;
				rect.y2 = y + 1;
				break;
			}
		}
		if (rect.y2 >= 0)
			break;
	}

	if (rect.y2 < 0)
		return rect; //return invalid rect as is, the bRect is all free

	// here we deliberately go in horizontal direction first because the GetGreedyRectangle was good to cut vertical blocks
	// this is a bad memory access patern though
	for (int y = rect.y2; y < bRect.y2; y++) {
		rect.y2 = y;
		size_t idx = y * sizeX + rect.x1;
		if (!BusyPred(updateContainer[idx]))
			break;
	}
#endif

	return rect;
}

void CRectangleOverlapHandler::ClearUpdateContainer(const SRectangle& rect)
{
	std::scoped_lock lock(*mutex); // exclusive lock for writing

	// cleanup the area occupied by the rect
	for (int y = rect.y1; y < rect.y2; ++y) {
		auto off = updateContainer.begin() + y * sizeX;
		auto beg = off + rect.x1;
		auto end = off + rect.x2;
		std::fill(beg, end, DataType::FREE);
	}
}