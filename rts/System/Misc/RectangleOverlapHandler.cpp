/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RectangleOverlapHandler.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Pair.h"

//#define ROH_MAKE_BITMAP_SNAPSHOTS
#ifdef ROH_MAKE_BITMAP_SNAPSHOTS
#include "Rendering/Textures/Bitmap.h"
#endif

#include <cassert>
#include <cmath>
#include <list>
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
	if (rectanglesVec.empty())
		return;

	if ((sizeX - 1) * (sizeY - 1) == areaLimit) {
		rectanglesVec.clear();
		rectanglesVec.emplace_back(0, 0, static_cast<int>(sizeX - 1), static_cast<int>(sizeY - 1));
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

	std::vector<SRectangle> boundRectangles;
	{
		static const auto Pred = [](auto val) { return (val == DataType::BUSY); };

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
				beg = std::find_if    (beg, fin, Pred);
				end = std::find_if_not(beg, fin, Pred);
				if (beg != end)
					lines.emplace_back(y, static_cast<int>(std::distance(stt, beg)), static_cast<int>(std::distance(stt, end)));

				beg = end; //rewind
			}
		}

		while (!lines.empty()) {
			auto beg = lines.begin();

			SRectangle bRect {
				beg->x1,
				beg->y,
				beg->x2,
				beg->y
			};
			int prevY = beg->y;
			beg = lines.erase(beg);

			for (auto it = beg; it != lines.end(); /*NOOP*/) {
				if (std::abs(it->y - prevY) > 1) {
					bRect.y2 = prevY;
					break;
				}

				prevY = it->y;

				const bool misalignedByX = (it->x2 < bRect.x1 || it->x1 > bRect.x2);
				if (misalignedByX) {
					++it;
					continue;
				}

				bRect.y2 = it->y;
				bRect.x1 = std::min(bRect.x1, it->x1);
				bRect.x2 = std::max(bRect.x2, it->x2);

				it = lines.erase(it); // advances it
			}

			// x2 is already non-inclusive, make y2 non-inclusive too
			++bRect.y2;
			boundRectangles.emplace_back(std::move(bRect));
		}
	}

	std::vector<int> heights(sizeX + 1, 0); // Include extra element for easier calculation
	std::vector<uint32_t> stack;
	stack.reserve(sizeX);
	size_t maxArea;

	for (const auto& bRect : boundRectangles) {
		do {
			SRectangle rect{
				std::numeric_limits<int16_t>::max(),
				std::numeric_limits<int16_t>::max(),
				std::numeric_limits<int16_t>::min(),
				std::numeric_limits<int16_t>::min()
			};

			maxArea = 0;

			for (uint32_t y = bRect.y1; y < bRect.y2; y++) {
				for (uint32_t x = bRect.x1; x < bRect.x2; x++) {
					auto itVal = updateContainer.begin() + y * sizeX + x;
					heights[x] = (*itVal == DataType::BUSY) ? heights[x] + 1 : 0;
				}

				// Calculate max area using stack-based method
				for (uint32_t x = std::max(bRect.x1 - 1, 0); x <= bRect.x2; x++) {
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

				stack.clear();
			}

			if (maxArea == 0)
				continue;

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

			rectanglesVec.emplace_back(rect.x1, rect.y1, rect.x2 - 1, rect.y2 - 1);

			std::fill(heights.begin() + bRect.x1, heights.begin() + bRect.x2 + 1, 0);
			assert(*std::max_element(heights.begin(), heights.end()) == 0);
#ifdef ROH_MAKE_BITMAP_SNAPSHOTS
			mem = bitmap.GetRawMem();
			for (auto value : updateContainer) {
				*mem = static_cast<uint8_t>(value == DataType::BUSY) * 0xFF;
				++mem;
			}
			bitmap.Save(fmt::format("CRectangleOverlapHandler-{}.png", step++), true);
#endif
		} while (maxArea > 0);
	}

}
