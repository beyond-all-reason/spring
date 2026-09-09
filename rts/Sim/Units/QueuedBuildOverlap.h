/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <vector>

#include "System/type2.h"

struct BuildInfo;

namespace QueuedBuildOverlap {

enum class Result {
	NONE,
	CANCEL,
	OVERLAP,
};

int GetYardMapIndex(int facing, const int2& yardPos, const int2& xrange, const int2& zrange);
bool IsInsideCancellationRectangle(const BuildInfo& earlier, const BuildInfo& proposed);

/// Add cells blocked by earlier to the proposed-build mask. Existing marked
/// cells are preserved so callers can accumulate multiple queued commands.
/// openCellCount must describe blockedCells and is updated by this call. Returns
/// true once every proposed-build cell is blocked.
bool AddBlockedCells(
	const BuildInfo& earlier,
	const BuildInfo& proposed,
	bool useYardmaps,
	std::vector<uint8_t>& blockedCells,
	size_t& openCellCount
);

/// Whether the earlier build would cancel or block the proposed build. The
/// occupied-cell test intentionally mirrors built-building yardmap rules.
Result Test(const BuildInfo& earlier, const BuildInfo& proposed, bool useYardmaps = true);

} // namespace QueuedBuildOverlap
