/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ExitOnlyMap.h"

#include <algorithm>

#include "map/ReadMap.h"
#include "System/SpringMath.h"

ExitOnlyMap exitOnlyMap;

// https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
int NextPowerOfTwo(int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}

void ExitOnlyMap::InitNewExitOnlyMap() {

    // May over allocate, but the size is based on single byte storage and allows for fast indexing.
    const int width = NextPowerOfTwo(std::max(mapDims.mapx, mapDims.mapy)) / resolution;

    stateMap.clear();
    stateMap.resize(Square(width), 0);
}

