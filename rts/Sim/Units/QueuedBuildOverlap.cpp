/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "QueuedBuildOverlap.h"

#include <algorithm>

#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "System/Misc/TracyDefs.h"
#include "System/SpringMath.h"

namespace QueuedBuildOverlap {

int GetYardMapIndex(int facing, const int2& yardPos, const int2& xrange, const int2& zrange)
{
	int yardX = yardPos.x - xrange.x;
	int yardZ = yardPos.y - zrange.x;
	int yardXR = xrange.y - xrange.x;
	int yardZR = zrange.y - zrange.x;

	switch (facing) {
		default: break;
		case FACING_NORTH: {
			yardX = yardXR - yardX - 1;
			yardZ = yardZR - yardZ - 1;
		} break;
		case FACING_EAST: {
			yardZ = yardZR - yardZ - 1;
			std::swap(yardX, yardZ);
			std::swap(yardXR, yardZR);
		} break;
		case FACING_WEST: {
			yardX = yardXR - yardX - 1;
			std::swap(yardX, yardZ);
			std::swap(yardXR, yardZR);
		} break;
	}

	return yardX + yardXR * yardZ;
}

bool IsInsideCancellationRectangle(const BuildInfo& earlier, const BuildInfo& proposed)
{
	if (earlier.def == nullptr || proposed.def == nullptr)
		return false;

	return (
		math::fabs(earlier.pos.x - proposed.pos.x) * 2 <= std::max(earlier.GetXSize(), proposed.GetXSize()) * SQUARE_SIZE &&
		math::fabs(earlier.pos.z - proposed.pos.z) * 2 <= std::max(earlier.GetZSize(), proposed.GetZSize()) * SQUARE_SIZE
	);
}

namespace {

class CellOverlapTest {
public:
	CellOverlapTest(const BuildInfo& earlier, const BuildInfo& proposed, bool useYardmaps)
		: earlier(earlier)
		, proposed(proposed)
		, earlierSize{earlier.GetXSize(), earlier.GetZSize()}
		, proposedSize{proposed.GetXSize(), proposed.GetZSize()}
		, earlierMins{
			int(earlier.pos.x / SQUARE_SIZE) - (earlierSize.x >> 1),
			int(earlier.pos.z / SQUARE_SIZE) - (earlierSize.y >> 1),
		}
		, proposedMins{
			int(proposed.pos.x / SQUARE_SIZE) - (proposedSize.x >> 1),
			int(proposed.pos.z / SQUARE_SIZE) - (proposedSize.y >> 1),
		}
		, earlierMaxs(earlierMins + earlierSize)
		, proposedMaxs(proposedMins + proposedSize)
		, overlapMins{std::max(earlierMins.x, proposedMins.x), std::max(earlierMins.y, proposedMins.y)}
		, overlapMaxs{std::min(earlierMaxs.x, proposedMaxs.x), std::min(earlierMaxs.y, proposedMaxs.y)}
		, earlierXRange{earlierMins.x, earlierMaxs.x}
		, earlierZRange{earlierMins.y, earlierMaxs.y}
		, proposedXRange{proposedMins.x, proposedMaxs.x}
		, proposedZRange{proposedMins.y, proposedMaxs.y}
		, rectangularFallback(
			!useYardmaps || earlier.def->yardmap.size() != static_cast<size_t>(earlier.def->xsize * earlier.def->zsize)
		)
		, proposedHasYardMap(
			proposed.def->yardmap.size() == static_cast<size_t>(proposed.def->xsize * proposed.def->zsize)
		)
	{}

	bool HasFootprintOverlap() const { return (overlapMins.x < overlapMaxs.x && overlapMins.y < overlapMaxs.y); }
	bool UsesRectangularFallback() const { return rectangularFallback; }

	bool BlocksCell(const int2& mapSquare) const
	{
		if (rectangularFallback)
			return true;

		const int earlierIndex = GetYardMapIndex(earlier.buildFacing, mapSquare, earlierXRange, earlierZRange);
		const YardMapStatus earlierStatus = earlier.def->yardmap[earlierIndex];

		YardMapStatus proposedStatus = YardmapStates::YARDMAP_BLOCKED;
		if (proposedHasYardMap) {
			const int proposedIndex = GetYardMapIndex(proposed.buildFacing, mapSquare, proposedXRange, proposedZRange);
			proposedStatus = proposed.def->yardmap[proposedIndex];
		}

		if (earlierStatus & (YardmapStates::YARDMAP_EXITONLY | YardmapStates::YARDMAP_UNBUILDABLE))
			return (proposedStatus > YardmapStates::YARDMAP_STACKABLE);
		if ((earlierStatus & YardmapStates::YARDMAP_BLOCKED) == 0)
			return false;
		if (proposedStatus <= YardmapStates::YARDMAP_GEOSTACKABLE)
			return false;
		if (earlierStatus == YardmapStates::YARDMAP_BUILDONLY)
			return false;

		return true;
	}

	int GetProposedCellIndex(const int2& mapSquare) const
	{
		return (mapSquare.y - proposedMins.y) * proposedSize.x + mapSquare.x - proposedMins.x;
	}

	const int2& GetOverlapMins() const { return overlapMins; }
	const int2& GetOverlapMaxs() const { return overlapMaxs; }
	int GetProposedCellCount() const { return proposedSize.x * proposedSize.y; }

private:
	const BuildInfo& earlier;
	const BuildInfo& proposed;
	const int2 earlierSize;
	const int2 proposedSize;
	const int2 earlierMins;
	const int2 proposedMins;
	const int2 earlierMaxs;
	const int2 proposedMaxs;
	const int2 overlapMins;
	const int2 overlapMaxs;
	const int2 earlierXRange;
	const int2 earlierZRange;
	const int2 proposedXRange;
	const int2 proposedZRange;
	const bool rectangularFallback;
	const bool proposedHasYardMap;
};

} // namespace

bool AddBlockedCells(
	const BuildInfo& earlier,
	const BuildInfo& proposed,
	bool useYardmaps,
	std::vector<uint8_t>& blockedCells,
	size_t& openCellCount
)
{
	if (earlier.def == nullptr || proposed.def == nullptr)
		return false;

	const CellOverlapTest overlapTest(earlier, proposed, useYardmaps);
	const size_t proposedCellCount = overlapTest.GetProposedCellCount();
	if (blockedCells.size() != proposedCellCount) {
		blockedCells.assign(proposedCellCount, false);
		openCellCount = proposedCellCount;
	}
	if (openCellCount == 0)
		return true;
	if (!overlapTest.HasFootprintOverlap())
		return false;

	const int2& overlapMins = overlapTest.GetOverlapMins();
	const int2& overlapMaxs = overlapTest.GetOverlapMaxs();
	const bool emptyMask = (openCellCount == proposedCellCount);
	for (int z = overlapMins.y; z < overlapMaxs.y; ++z) {
		if (overlapTest.UsesRectangularFallback() && emptyMask) {
			const int firstIndex = overlapTest.GetProposedCellIndex({overlapMins.x, z});
			const int rowCellCount = overlapMaxs.x - overlapMins.x;
			std::fill_n(blockedCells.begin() + firstIndex, rowCellCount, true);
			openCellCount -= rowCellCount;
			continue;
		}

		for (int x = overlapMins.x; x < overlapMaxs.x; ++x) {
			const int2 mapSquare = {x, z};
			uint8_t& blockedCell = blockedCells[overlapTest.GetProposedCellIndex(mapSquare)];
			if (blockedCell || !overlapTest.BlocksCell(mapSquare))
				continue;

			blockedCell = true;
			if (--openCellCount == 0)
				return true;
		}
	}

	return (openCellCount == 0);
}

Result Test(const BuildInfo& earlier, const BuildInfo& proposed, bool useYardmaps)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (earlier.def == nullptr || proposed.def == nullptr)
		return Result::NONE;

	const CellOverlapTest overlapTest(earlier, proposed, useYardmaps);
	if (!overlapTest.HasFootprintOverlap())
		return Result::NONE;

	const Result overlapResult = IsInsideCancellationRectangle(earlier, proposed) ? Result::CANCEL : Result::OVERLAP;
	if (overlapTest.UsesRectangularFallback())
		return overlapResult;

	const int2& overlapMins = overlapTest.GetOverlapMins();
	const int2& overlapMaxs = overlapTest.GetOverlapMaxs();
	for (int z = overlapMins.y; z < overlapMaxs.y; ++z) {
		for (int x = overlapMins.x; x < overlapMaxs.x; ++x) {
			if (overlapTest.BlocksCell({x, z}))
				return overlapResult;
		}
	}

	return Result::NONE;
}

} // namespace QueuedBuildOverlap
