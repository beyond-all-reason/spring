/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/type2.h"

struct BuildInfo;

namespace QueuedBuildOverlap {

int GetYardMapIndex(int facing, const int2& yardPos, const int2& xrange, const int2& zrange);
bool IsInsideCancellationRectangle(const BuildInfo& earlier, const BuildInfo& proposed);

/// Whether the earlier footprint would block the proposed footprint if it
/// already existed. This intentionally mirrors built-building yardmap rules.
bool Test(const BuildInfo& earlier, const BuildInfo& proposed);

} // namespace QueuedBuildOverlap
