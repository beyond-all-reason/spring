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

Result Test(const BuildInfo& earlier, const BuildInfo& proposed, bool useYardmaps)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (earlier.def == nullptr || proposed.def == nullptr)
		return Result::NONE;

	const int2 earlierSize = {earlier.GetXSize(), earlier.GetZSize()};
	const int2 proposedSize = {proposed.GetXSize(), proposed.GetZSize()};
	const int2 earlierMins = {
		int(earlier.pos.x / SQUARE_SIZE) - (earlierSize.x >> 1),
		int(earlier.pos.z / SQUARE_SIZE) - (earlierSize.y >> 1),
	};
	const int2 proposedMins = {
		int(proposed.pos.x / SQUARE_SIZE) - (proposedSize.x >> 1),
		int(proposed.pos.z / SQUARE_SIZE) - (proposedSize.y >> 1),
	};
	const int2 earlierMaxs = earlierMins + earlierSize;
	const int2 proposedMaxs = proposedMins + proposedSize;
	const int overlapX1 = std::max(earlierMins.x, proposedMins.x);
	const int overlapX2 = std::min(earlierMaxs.x, proposedMaxs.x);
	const int overlapZ1 = std::max(earlierMins.y, proposedMins.y);
	const int overlapZ2 = std::min(earlierMaxs.y, proposedMaxs.y);

	if (overlapX1 >= overlapX2 || overlapZ1 >= overlapZ2)
		return Result::NONE;

	const Result overlapResult = IsInsideCancellationRectangle(earlier, proposed) ? Result::CANCEL : Result::OVERLAP;
	if (!useYardmaps)
		return overlapResult;

	const size_t earlierYardMapSize = earlier.def->xsize * earlier.def->zsize;
	const size_t proposedYardMapSize = proposed.def->xsize * proposed.def->zsize;

	if (earlier.def->yardmap.size() != earlierYardMapSize)
		return overlapResult;

	const int2 earlierXRange = {earlierMins.x, earlierMaxs.x};
	const int2 earlierZRange = {earlierMins.y, earlierMaxs.y};
	const int2 proposedXRange = {proposedMins.x, proposedMaxs.x};
	const int2 proposedZRange = {proposedMins.y, proposedMaxs.y};
	const bool proposedHasYardMap = proposed.def->yardmap.size() == proposedYardMapSize;

	for (int z = overlapZ1; z < overlapZ2; ++z) {
		for (int x = overlapX1; x < overlapX2; ++x) {
			const int2 yardPos = {x, z};
			const int earlierIndex = GetYardMapIndex(earlier.buildFacing, yardPos, earlierXRange, earlierZRange);
			const YardMapStatus earlierStatus = earlier.def->yardmap[earlierIndex];

			YardMapStatus proposedStatus = YardmapStates::YARDMAP_BLOCKED;
			if (proposedHasYardMap) {
				const int proposedIndex = GetYardMapIndex(proposed.buildFacing, yardPos, proposedXRange, proposedZRange);
				proposedStatus = proposed.def->yardmap[proposedIndex];
			}

			if (earlierStatus & (YardmapStates::YARDMAP_EXITONLY | YardmapStates::YARDMAP_UNBUILDABLE)) {
				if (proposedStatus > YardmapStates::YARDMAP_STACKABLE)
					return overlapResult;
				continue;
			}

			if ((earlierStatus & YardmapStates::YARDMAP_BLOCKED) == 0)
				continue;
			if (proposedStatus <= YardmapStates::YARDMAP_GEOSTACKABLE)
				continue;
			if (earlierStatus == YardmapStates::YARDMAP_BUILDONLY)
				continue;

			return overlapResult;
		}
	}

	return Result::NONE;
}

} // namespace QueuedBuildOverlap
