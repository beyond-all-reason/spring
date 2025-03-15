/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RectangleOverlapHandler.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Pair.h"
#include "Rendering/Textures/Bitmap.h"

#include <cassert>
#include <numeric>
#include <fmt/format.h>

CR_BIND(CRectangleOverlapHandler, )
CR_REG_METADATA(CRectangleOverlapHandler, (
	CR_MEMBER(sizeX),
	CR_MEMBER(sizeY),
	CR_MEMBER(updateContainer),
	CR_MEMBER(rectanglesVec)
))

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

void CRectangleOverlapHandler::Process(size_t areaLimit)
{
	if ((sizeX - 1) * (sizeY - 1) == areaLimit) {
		rectanglesVec.clear();
		rectanglesVec.emplace_back(0, 0, sizeX - 1, sizeY - 1);
		std::fill(updateContainer.begin(), updateContainer.end(), DataType::FREE);
		return;
	}

	for (const auto& rect : rectanglesVec) {
		for (int y = rect.y1; y <= rect.y2; ++y) {
			auto off = updateContainer.begin() + y * sizeX;
			auto beg = off + rect.x1 + 0;
			auto end = off + rect.x2 + 1;
			std::fill(beg, end, DataType::BUSY);
		}
	}
	rectanglesVec.clear();

#if 0
	size_t step = 0;
	CBitmap bitmap(nullptr, sizeX, sizeY, 1);
	auto* mem = bitmap.GetRawMem();
	for (auto value : updateContainer) {
		*mem = static_cast<uint8_t>(value == DataType::BUSY) * 0xFF;
		++mem;
	}
    bitmap.Save(fmt::format("CRectangleOverlapHandler-{}.png", step++), true);
#endif

	std::vector<int> heights(sizeX + 1, 0); // Include extra element for easier calculation
	std::vector<uint32_t> stack;
	stack.reserve(sizeX);
	size_t maxArea;

	do {
		SRectangle rect {
			std::numeric_limits<int16_t>::max(),
			std::numeric_limits<int16_t>::max(),
			std::numeric_limits<int16_t>::min(),
			std::numeric_limits<int16_t>::min()
		};

		maxArea = 0;
		std::fill(heights.begin(), heights.end(), 0);
		stack.clear();

		for (uint32_t y = 0; y < sizeY; y++) {
			for (uint32_t x = 0; x < sizeX; x++) {
				auto itVal = updateContainer.begin() + y * sizeX + x;
				heights[x] = (*itVal == DataType::BUSY) ? heights[x] + 1 : 0;
			}

			// Calculate max area using stack-based method
			for (uint32_t x = 0; x <= sizeX; x++) {
				while (!stack.empty() && heights[x] < heights[stack.back()]) {
					const uint32_t h = heights[stack.back()];
					stack.pop_back();
					const uint32_t w = stack.empty() ? x : x - stack.back() - 1;
					const size_t area = h * w;
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
		}

		// make non-inclusive
		++rect.x2;
		++rect.y2;

		// cleanup the area occupied by the rect
		for (int y = rect.y1; y < rect.y2; ++y) {
			auto off = updateContainer.begin() + y * sizeX;
			auto beg = off + rect.x1;
			auto end = off + rect.x2;
			std::fill(beg, end, DataType::FREE);
		}

#if 0
		mem = bitmap.GetRawMem();
		for (auto value : updateContainer) {
			*mem = static_cast<uint8_t>(value == DataType::BUSY) * 0xFF;
			++mem;
		}
		bitmap.Save(fmt::format("CRectangleOverlapHandler-{}.png", step++), true);
#endif
	} while (maxArea > 0);

}
