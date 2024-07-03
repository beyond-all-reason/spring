/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "YardmapStatusEffectsMap.h"

#include <algorithm>
#include <bit>

#include "Map/ReadMap.h"
#include "System/SpringMath.h"

YardmapStatusEffectsMap yardmapStatusEffectsMap;

CR_BIND(YardmapStatusEffectsMap, )

CR_REG_METADATA(YardmapStatusEffectsMap, (
	CR_MEMBER(stateMap)
))

void YardmapStatusEffectsMap::InitNewYardmapStatusEffectsMap() {
    // May over allocate, but the size is based on single byte storage and allows for fast indexing.
    const auto width = std::bit_ceil<uint32_t>(std::max(mapDims.mapx, mapDims.mapy));

    stateMap.clear();
    stateMap.resize(Square(width), 0);
}

