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
	CR_MEMBER(updateContainer),
	CR_MEMBER(rectanglesVec)
))

void CRectangleOverlapHandler::push_back(const SRectangle& rect)
{
	if (updateContainer.empty())
		return;

	if (rect.GetArea() <= 0)
		return;

	for (int y = rect.y1; y <= rect.y2; ++y) {
		auto off = updateContainer.begin() + y * sizeX;
		auto beg = off + rect.x1 + 0;
		auto end = off + rect.x2 + 1;
		std::fill(beg, end, BUSY);
	}

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
}

void CRectangleOverlapHandler::Process(size_t maxArea)
{
	if ((sizeX - 1) * (sizeY - 1) == maxArea) {
		rectanglesVec.emplace_back(0, 0, sizeX - 1, sizeY - 1);
		std::fill(updateContainer.begin(), updateContainer.end(), FREE);
		// isEmpty will be set in pop_front_n
		return;
	}

#if 1
	size_t step = 0;
	CBitmap bitmap(nullptr, sizeX, sizeY, 1);
	auto* mem = bitmap.GetRawMem();
	for (auto value : updateContainer) {
		*mem = static_cast<uint8_t>(value == BUSY) * 0xFF;
		++mem;
	}
    bitmap.Save(fmt::format("CRectangleOverlapHandler-{}.bmp", step++), true);
#endif

	SRectangle minMax {
		std::numeric_limits<int>::max(),
		std::numeric_limits<int>::max(),
		std::numeric_limits<int>::min(),
		std::numeric_limits<int>::min(),
	};

	for (int y = 0; y < sizeY; ++y) {
		for (int x = 0; x < sizeX; ++x) {
			size_t idx = y * sizeX + x;
			if (updateContainer[idx] == BUSY) {
				minMax.x1 = std::min(minMax.x1, x);
				minMax.y1 = std::min(minMax.y1, y);
				minMax.x2 = std::max(minMax.x2, x);
				minMax.y2 = std::max(minMax.y2, y);
			}
		}
	}

	// x2, y2 are non-inclusive
	++minMax.x2;
	++minMax.y2;

	assert(minMax.GetArea() > 0);

	// in all rectangles here x2, y2 are non-inclusive
	std::vector<SRectangle> tmp;

	tmp.emplace_back(std::move(minMax));

	while (!tmp.empty()) {
		const SRectangle rect = tmp.back(); tmp.pop_back();

		const auto W = rect.GetWidth();
		const auto H = rect.GetHeight();

		if (W * H <= maxArea) {
			bool allBusy = true;
			bool allFree = true;
			for (int y = rect.y1; y < rect.y2; ++y) {
				auto off = updateContainer.begin() + y * sizeX;
				auto beg = off + rect.x1;
				auto end = off + rect.x2;
				for (auto it = beg; it != end; ++it) {
					allBusy &= ~(*it == FREE);
					allFree &= ~(*it == BUSY);

					if (!allBusy && !allFree)
						break;
				}

				if (!allBusy && !allFree)
					break;
			}

			if (allFree) {
				// useless rectangle, just skip
				continue; // the while() loop
			}
			
			if (allBusy) {
				// useful rectangle, will add to rectanglesVec

				// cleanup the area occupied by the rect
				for (int y = rect.y1; y < rect.y2; ++y) {
					auto off = updateContainer.begin() + y * sizeX;
					auto beg = off + rect.x1;
					auto end = off + rect.x2;
					std::fill(beg, end, FREE); 
				}

#if 1
				CBitmap bitmap(nullptr, sizeX, sizeY, 1);
				auto* mem = bitmap.GetRawMem();
				for (auto value : updateContainer) {
					*mem = static_cast<uint8_t>(value == BUSY) * 0xFF;
					++mem;
				}
				bitmap.Save(fmt::format("CRectangleOverlapHandler-{}.bmp", step++), true);
#endif

				// inclusive x2 and y2
				rectanglesVec.emplace_back(rect.x1, rect.y1, rect.x2 - 1, rect.y2 - 1);

				continue; // the while() loop
			}
		}

		SRectangle subRectA;
		SRectangle subRectB;

		if (W >= H) {
			int mid = rect.x1 + W / 2;
			subRectA = SRectangle(rect.x1, rect.y1, mid    , rect.y2);
			subRectB = SRectangle(mid    , rect.y1, rect.x2, rect.y2);
		}
		else {
			int mid = rect.y1 + H / 2;
			subRectA = SRectangle(rect.x1, rect.y1, rect.x2, mid    );
			subRectB = SRectangle(rect.x1, mid    , rect.x2, rect.y2);
		}

		assert(W * H == subRectA.GetArea() + subRectB.GetArea());
		tmp.emplace_back(std::move(subRectA));
		tmp.emplace_back(std::move(subRectB));
	}

	std::sort(rectanglesVec.begin(), rectanglesVec.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.GetArea() > rhs.GetArea();
	});
}
