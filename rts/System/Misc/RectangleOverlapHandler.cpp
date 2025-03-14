/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RectangleOverlapHandler.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Pair.h"
#include "Rendering/Textures/Bitmap.h"

#include <cassert>
#include <fmt/format.h>

CR_BIND(CRectangleOverlapHandler, )
CR_REG_METADATA(CRectangleOverlapHandler, (
	CR_MEMBER(sizeX),
	CR_MEMBER(sizeY),
	CR_MEMBER(updateCounter),
	CR_MEMBER(updateContainer),
	CR_MEMBER(rectanglesVec)
))

void CRectangleOverlapHandler::push_back(const SRectangle& rect)
{
	if (updateContainer.empty())
		return;

	if (rect.GetArea() <= 0)
		return;

	bool updated = false;
	for (int y = rect.y1; y <= rect.y2; ++y) {
		for (int x = rect.x1; x <= rect.x2; ++x) {
			size_t idx = y * sizeX + x;
			auto& currValue = updateContainer[idx];
			updated |= (currValue > updateCounter);
			assert(currValue != updateCounter);
			currValue = std::min(currValue, updateCounter);
		}
	}

	updateCounter += static_cast<uint64_t>(updated);
	isEmpty = false;
}

void CRectangleOverlapHandler::pop_front_n(size_t n)
{
	if (rectanglesVec.empty())
		return;

	decltype(rectanglesVec) tmpMap;
	tmpMap.assign(rectanglesVec.begin() + n, rectanglesVec.end());
	rectanglesVec = std::move(tmpMap);

	isEmpty = rectanglesVec.empty();
	if (isEmpty) {
		assert(*std::min_element(updateContainer.begin(), updateContainer.end()) == EMPTY);
		updateCounter = 0; // safe to do
	}
}

void CRectangleOverlapHandler::Process(size_t maxArea, size_t maxUnoccupied, float maxUnoccupiedPerc)
{
	if ((sizeX - 1) * (sizeY - 1) == maxArea) {
		rectanglesVec.emplace_back(0, 0, sizeX - 1, sizeY - 1);
		std::fill(updateContainer.begin(), updateContainer.end(), EMPTY);
		// isEmpty will be set in pop_front_n
		return;
	}

	int step = 0;
	while(true) {
#if 1
		CBitmap bitmap(nullptr, sizeX, sizeY, 1);
		auto* mem = bitmap.GetRawMem();
		for (auto value : updateContainer) {
			*mem = (value != EMPTY) * 0xFF;
			++mem;
		}
		bitmap.Save(fmt::format("CRectangleOverlapHandler-{}.bmp", step), true);
#endif

		size_t x1, y1;
		{
			auto it = std::min_element(updateContainer.begin(), updateContainer.end());
			if (*it == EMPTY)
				return;

			size_t idx = std::distance(updateContainer.begin(), it);
			y1 = idx / sizeX;
			x1 = idx % sizeX;
		}

		size_t x2 = x1;
		size_t y2 = y1;
		size_t numUnoccupied = 0;

		for (bool failedOnDirX = false, failedOnDirY = false; !failedOnDirX && !failedOnDirY; /*NOOP*/) {
			size_t currW = x2 - x1;
			size_t currH = y2 - y1;
			size_t currArea = currW * currH;

			size_t numAddUnoccupied = 0;

			if (currW >= currH && !failedOnDirX) {
				const size_t tmpCurrArea = currArea + currW;
				if (currArea + currW > maxArea || y2 >= sizeY) {
					failedOnDirX = true;
					continue;
				}
				for (size_t x = x1; x <= x2; ++x) {
					size_t idx = (y2 + 1) * sizeX + (x);
					numAddUnoccupied += (updateContainer[idx] == EMPTY);
				}

				// don't allow full row of empties
				if (numAddUnoccupied > 0 && numAddUnoccupied == currW + 1) {
					failedOnDirX = true;
					continue;
				}

				const size_t tmpNumOccupied = numUnoccupied + numAddUnoccupied;
				const float unoccupiedPerc = static_cast<float>(tmpNumOccupied) / std::max(tmpCurrArea, size_t(1));
				if (tmpNumOccupied > maxUnoccupied && unoccupiedPerc > maxUnoccupiedPerc) {
					failedOnDirX = true;
					continue;
				}

				numUnoccupied = tmpNumOccupied;
				++y2;
			}
			else if (!failedOnDirY) {
				const size_t tmpCurrArea = currArea + currH;
				if (tmpCurrArea > maxArea || x2 >= sizeX) {
					failedOnDirY = true;
					continue;
				}
				for (size_t y = y1; y <= y2; ++y) {
					size_t idx = (y)*sizeX + (x2 + 1);
					numAddUnoccupied += (updateContainer[idx] == EMPTY);
				}

				// don't allow full collumn of empties
				if (numAddUnoccupied > 0 && numAddUnoccupied == currH + 1) {
					failedOnDirY = true;
					continue;
				}

				const size_t tmpNumOccupied = numUnoccupied + numAddUnoccupied;
				const float unoccupiedPerc = static_cast<float>(tmpNumOccupied) / tmpCurrArea;
				if (tmpNumOccupied > maxUnoccupied && unoccupiedPerc > maxUnoccupiedPerc) {
					failedOnDirY = true;
					continue;
				}

				numUnoccupied = tmpNumOccupied;
				++x2;
			}
		}

		for (size_t y = y1; y <= y2; ++y) {
			auto off = updateContainer.begin() + y * sizeX;
			auto beg = off + x1    ;
			auto end = off + x2 + 1;
			std::fill(beg, end, EMPTY);
		}

		rectanglesVec.emplace_back(
			static_cast<int>(x1),
			static_cast<int>(y1),
			static_cast<int>(x2),
			static_cast<int>(y2)
		);

		step++;
	}
}
