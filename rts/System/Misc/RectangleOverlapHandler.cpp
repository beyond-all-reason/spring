/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RectangleOverlapHandler.h"
#include "System/Log/ILog.h"
#include "System/creg/STL_Pair.h"

#include <cassert>

CR_BIND(CRectangleOverlapHandler, )
CR_REG_METADATA(CRectangleOverlapHandler, (
	CR_MEMBER(sizeX),
	CR_MEMBER(sizeY),
	CR_MEMBER(maxSideSize),
	CR_MEMBER(updateCounter),
	CR_MEMBER(updateContainer),
	CR_MEMBER(rectanglesVec)
))

void CRectangleOverlapHandler::push_back(const SRectangle& rect, bool noSplit)
{
	if (updateContainer.empty())
		return;

	if (rect.GetArea() <= 0)
		return;

	const int currMaxSideSize = noSplit ? (1 << 24) : maxSideSize;

	for (int y1 = rect.y1; y1 < rect.y2; y1 += currMaxSideSize) {
		int y2 = std::min(y1 + currMaxSideSize, rect.y2);

		for (int x1 = rect.x1; x1 < rect.x2; x1 += currMaxSideSize) {
			int x2 = std::min(x1 + currMaxSideSize, rect.x2);

			const auto searchValue = std::pair{ updateCounter, SRectangle{} };

			auto it = std::lower_bound(rectanglesVec.begin(), rectanglesVec.end(), searchValue,
				[](const auto& lhs, const auto& rhs) {
					return lhs.first < rhs.first;
			});

			for (int y = y1; y <= y2; ++y) {
				for (int x = x1; x <= x2; ++x) {
					size_t idx = y * sizeX + x;
					auto& currValue = updateContainer[idx];
					if (currValue > searchValue.first) {
						currValue = searchValue.first;

						if unlikely(it == rectanglesVec.end() || it->first != searchValue.first) {
							it = rectanglesVec.insert(it, searchValue);
							++updateCounter;
						}

						it->second.x1 = std::min(it->second.x1, x);
						it->second.y1 = std::min(it->second.y1, y);
						it->second.x2 = std::max(it->second.x2, x);
						it->second.y2 = std::max(it->second.y2, y);
					}
				}
			}
		}

	}
}

void CRectangleOverlapHandler::pop_front_n(size_t n)
{
	if (rectanglesVec.empty())
		return;

	for (size_t i = 0; i < std::min(n, rectanglesVec.size()); ++i) {
		const auto& [rectUpdID, rect] = rectanglesVec[i];
		for (int y = rect.y1; y <= rect.y2; ++y) {
			for (int x = rect.x1; x <= rect.x2; ++x) {
				size_t idx = y * sizeX + x;
				auto& currValue = updateContainer[idx];
				assert(currValue >= rectUpdID); // rectUpdID or EMPTY
				if (currValue == rectUpdID)
					currValue = EMPTY;
			}
		}
	}

	RectanglesVecType tmpMap;
	tmpMap.assign(rectanglesVec.begin() + n, rectanglesVec.end());
	rectanglesVec = std::move(tmpMap);
}
