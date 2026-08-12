/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

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

/// Whether the earlier build would cancel or block the proposed build. The
/// occupied-cell test intentionally mirrors built-building yardmap rules.
Result Test(const BuildInfo& earlier, const BuildInfo& proposed, bool useYardmaps = true);

} // namespace QueuedBuildOverlap
