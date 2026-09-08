/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "YardmapStatusEffectsMap.h"

#include <algorithm>
#include <cassert>
#include <cstring>

YardmapStatusEffectsMap yardmapStatusEffectsMap;

CR_BIND(YardmapStatusEffectsMap, )

CR_REG_METADATA(YardmapStatusEffectsMap, (
	CR_MEMBER(stateMap),
	CR_IGNORED(tileStride),   // rebuilt from stateMap size on PostLoad
	CR_POSTLOAD(PostLoad)
))

CR_BIND(YardmapStatusEffectsMap::Tile, )

CR_REG_METADATA(YardmapStatusEffectsMap::Tile, (
	CR_MEMBER(squares)
))

void YardmapStatusEffectsMap::ClearTile(int tileId) {
	assert(tileId >= 0 && tileId < static_cast<int>(stateMap.size()));
	memset(&stateMap[tileId], 0, sizeof(Tile));
}

void YardmapStatusEffectsMap::InitNewYardmapStatusEffectsMap() {
	const int tilesX = (mapDims.mapx + TILE_SIZE - 1) / TILE_SIZE;
	const int tilesZ = (mapDims.mapy + TILE_SIZE - 1) / TILE_SIZE;

	tileStride = tilesX;

	stateMap.clear();
	stateMap.resize(tilesX * tilesZ); // value-initialises: squares are zeroed
}

void YardmapStatusEffectsMap::PostLoad() {
	tileStride = (mapDims.mapx + TILE_SIZE - 1) / TILE_SIZE;
}