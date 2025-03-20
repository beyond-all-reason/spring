/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RectangleOverlapHandler.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Pair.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"

#include <cassert>
#include <array>
#include <queue>
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
	CR_IGNORED(rectOwnersContainer),
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

	std::array<decltype(rectanglesVec), ThreadPool::MAX_THREADS> perThreadRectangles;

	auto boundingRectsData = GetBoundingRectsData();
	int totalSum = 0;
	for (const auto& brd : boundingRectsData)
		totalSum += brd.second;

	const auto MainLoopBody = [&, this](int bri) {
		auto& [bRect, occArea] = boundingRectsData[bri];
		const auto bRectArea = bRect.GetArea();

		enum RectDecompositionAlgo {
			MAXIMAL,
			GREEDY,
			LINE
		};
		RectDecompositionAlgo algo = MAXIMAL;

		static constexpr auto MAXIMAL_ALGO_AREA_THRESHOLD = 0.25f;
		static constexpr auto GREEDY_ALGO_SPARSITY_THRESHOLD = 0.02f;

		while (true) {
			SRectangle rect;

			switch (algo)
			{
			case MAXIMAL:
				rect = GetMaximalRectangle(bRect, bri);
				break;
			case GREEDY:
				rect = GetGreedyRectangle(bRect, bri);
				break;
			case LINE:
				rect = GetLineRectangle(bRect, bri);
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
			assert(occArea >= 0);

			if (algo == RectDecompositionAlgo::MAXIMAL && static_cast<float>(area) / bRectArea < MAXIMAL_ALGO_AREA_THRESHOLD) {
				algo = RectDecompositionAlgo::GREEDY;
			}
			//else if (algo == RectDecompositionAlgo::GREEDY && static_cast<float>(occArea) / bRectArea < GREEDY_ALGO_SPARSITY_THRESHOLD) {
			//	algo = RectDecompositionAlgo::LINE;
			//}

			ClearUpdateContainer(rect);

			perThreadRectangles[ThreadPool::GetThreadNum()].emplace_back(rect.x1, rect.y1, rect.x2 - 1, rect.y2 - 1);
		}
	};

	for_mt(0, static_cast<int>(boundingRectsData.size()), MainLoopBody);

	// sanity checks
	assert(std::all_of(boundingRectsData.begin(), boundingRectsData.end(), [](const auto& item) { return item.second == 0; }));
	assert(*std::max_element(updateContainer.begin(), updateContainer.end()) != DataType::BUSY);
	assert(rectanglesVec.empty());

	// cleanup
	std::fill(rectOwnersContainer.begin(), rectOwnersContainer.end(), -1);

	for (const auto& thisThreadRectangles : perThreadRectangles) {
		for (const auto& rect : thisThreadRectangles) {
			statsOutputArea += rect.GetArea();
			rectanglesVec.emplace_back(rect);
		}
	}

	statsOutputRects += rectanglesVec.size();
}

std::vector<std::pair<SRectangle, int>> CRectangleOverlapHandler::GetBoundingRectsData()
{
	ZoneScoped;

	std::vector<std::pair<SRectangle, int>> boundingRectsData;

	static const std::array directions = {
		std::make_pair<int, int>(-1,  0),
		std::make_pair<int, int>( 1,  0),
		std::make_pair<int, int>( 0, -1),
		std::make_pair<int, int>( 0,  1)
	};

	decltype(updateContainer) updateContainerCopy = updateContainer;

	for (int y = 0; y < sizeY; ++y) {
		int baseIdx = y * sizeX;
		for (int x = 0; x < sizeX; ++x) {		
			int idx = baseIdx + x;
			if (updateContainerCopy[idx] == DataType::BUSY) {
				std::queue<std::pair<int, int>> q;
				q.push({ x, y });
				updateContainerCopy[idx] = DataType::FREE;

				const int bRectNum = static_cast<int>(boundingRectsData.size());
				SRectangle bRect = { x, y, x, y }; // degenerate rectangle, will get fixed in the loop below
				int occArea = 0;

				while (!q.empty()) {
					auto [cx, cy] = q.front();
					q.pop();

					bRect.x1 = std::min(bRect.x1, cx    );
					bRect.x2 = std::max(bRect.x2, cx + 1);
					bRect.y1 = std::min(bRect.y1, cy    );
					bRect.y2 = std::max(bRect.y2, cy + 1);
					rectOwnersContainer[cy * sizeX + cx] = bRectNum;
					++occArea;

					for (auto [dx, dy] : directions) {
						const int nx = cx + dx;
						const int ny = cy + dy;
						if (nx >= 0 && nx < sizeX && ny >= 0 && ny < sizeY) {
							int nidx = ny * sizeX + nx;
							if (updateContainerCopy[nidx] == DataType::BUSY) {
								q.push({ nx, ny });
								updateContainerCopy[nidx] = DataType::FREE;
							}
						}
					}
				}
				boundingRectsData.emplace_back(bRect, occArea);
			}
		}
	}

	return boundingRectsData;
}

SRectangle CRectangleOverlapHandler::GetMaximalRectangle(const SRectangle& bRect, int bRectNum) const
{
	ZoneScoped;

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
		int baseIdx = y * sizeX;
		for (int x = bRect.x1; x < bRect.x2; x++) {
			int idx = baseIdx + x;
			heights[x] = (updateContainer[idx] == DataType::BUSY && rectOwnersContainer[idx] == bRectNum) ? heights[x] + 1 : 0;
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

SRectangle CRectangleOverlapHandler::GetGreedyRectangle(const SRectangle& bRect, int bRectNum) const
{
	ZoneScoped;

	SRectangle rect {
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::min(),
		std::numeric_limits<int16_t>::min()
	};

	const auto ValidOwnerPred = [bRectNum](const auto val) { return bRectNum == val; };

	for (int y = bRect.y1; y < bRect.y2; y++) {
		int baseIdx = y * sizeX;
		int x1 = -1;
		int x2 = -1;

		// Find first busy column
		for (int x = bRect.x1; x < bRect.x2; x++) {
			int idx = baseIdx + x;

			if (!ValidOwnerPred(rectOwnersContainer[idx]))
				continue;

			if (BusyPred(updateContainer[idx])) {
				x1 = x;
				break;
			}
		}

		if (x1 == -1)
			continue;

		// Find first non-busy column after start
		x2 = bRect.x2;
		for (int x = x1 + 1; x < bRect.x2; x++) {
			int idx = baseIdx + x;

			if (!BusyPred(updateContainer[idx])) {
				x2 = x;
				break;
			}
		}

		rect.x1 = x1;
		rect.x2 = x2;
		rect.y1 = y;
		rect.y2 = y + 1;
		break;
	}

	if (rect.y2 < 0)
		return rect;

	for (int y = rect.y2; y < bRect.y2; y++) {
		int baseIdx = y * sizeX;

		bool validRange =
			std::all_of(rectOwnersContainer.begin() + baseIdx + rect.x1, rectOwnersContainer.begin() + baseIdx + rect.x2, ValidOwnerPred) &&
			std::all_of(updateContainer.begin()     + baseIdx + rect.x1, updateContainer.begin()     + baseIdx + rect.x2, BusyPred);

		if (validRange)
			rect.y2 = y + 1;
		else
			break;
	}

	return rect;
}

SRectangle CRectangleOverlapHandler::GetLineRectangle(const SRectangle& bRect, int bRectNum) const
{
	ZoneScoped;

	SRectangle rect{
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::max(),
		std::numeric_limits<int16_t>::min(),
		std::numeric_limits<int16_t>::min()
	};

	const auto ValidOwnerPred = [bRectNum](const auto val) { return bRectNum == val; };

	for (int y = bRect.y1; y < bRect.y2; y++) {
		int baseIdx = y * sizeX;
		int x1 = -1;
		int x2 = -1;

		// Find first busy column
		for (int x = bRect.x1; x < bRect.x2; x++) {
			int idx = baseIdx + x;

			if (!ValidOwnerPred(rectOwnersContainer[idx]))
				continue;

			if (BusyPred(updateContainer[idx])) {
				x1 = x;
				break;
			}
		}

		if (x1 == -1)
			continue;

		// Find first non-busy column after start
		x2 = bRect.x2;
		for (int x = x1 + 1; x < bRect.x2; x++) {
			int idx = baseIdx + x;

			if (!BusyPred(updateContainer[idx])) {
				x2 = x;
				break;
			}
		}

		rect.x1 = x1;
		rect.x2 = x2;
		rect.y1 = y;
		rect.y2 = y + 1;
		break;
	}

	return rect;
}

void CRectangleOverlapHandler::ClearUpdateContainer(const SRectangle& rect)
{
	// cleanup the area occupied by the rect
	for (int y = rect.y1; y < rect.y2; ++y) {
		auto off = updateContainer.begin() + y * sizeX;
		auto beg = off + rect.x1;
		auto end = off + rect.x2;
		std::fill(beg, end, DataType::FREE);
	}
}