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
	CR_IGNORED(eoStride),     // derived from stateMap, rebuilt on PostLoad
	CR_IGNORED(eoBlocks),     // ditto — never the source of truth
	CR_POSTLOAD(PostLoad)
))

CR_BIND(YardmapStatusEffectsMap::Tile, )

CR_REG_METADATA(YardmapStatusEffectsMap::Tile, (
	CR_MEMBER(squares)
))

void YardmapStatusEffectsMap::ClearTile(int tileId) {
	assert(tileId >= 0 && tileId < static_cast<int>(stateMap.size()));

	// A tile spans 8x8 squares and a coarse block 16x16, both aligned, so every
	// square of a tile lives in the same block: count the EXIT_ONLY squares
	// about to be erased and correct that block once.
	const Tile& tile = stateMap[tileId];
	int erased = 0;
	for (int i = 0; i < TILE_AREA; ++i)
		erased += ((tile.squares[i] & EXIT_ONLY) != 0);

	if (erased != 0) {
		const int tx = (tileId % tileStride) * TILE_SIZE;
		const int tz = (tileId / tileStride) * TILE_SIZE;
		eoBlocks[EOBlockIdx(tx, tz)] -= erased;
	}

	memset(&stateMap[tileId], 0, sizeof(Tile));
}

// Recompute the coarse EXIT_ONLY grid from the fine map. Called on map init
// (where it just sizes and zeroes it) and after deserialization, since the grid
// is derived state and is not serialized.
void YardmapStatusEffectsMap::RebuildExitOnlyBlocks() {
	eoStride = ((mapDims.mapx - 1) >> EO_BLOCK_SHIFT) + 1;
	const int blocksZ = ((mapDims.mapy - 1) >> EO_BLOCK_SHIFT) + 1;
	eoBlocks.assign(eoStride * blocksZ, 0);

	for (int z = 0; z < mapDims.mapy; ++z) {
		for (int x = 0; x < mapDims.mapx; ++x) {
			if (GetMapState(x, z) & EXIT_ONLY)
				++eoBlocks[EOBlockIdx(x, z)];
		}
	}
}

void YardmapStatusEffectsMap::InitNewYardmapStatusEffectsMap() {
	const int tilesX = (mapDims.mapx + TILE_SIZE - 1) / TILE_SIZE;
	const int tilesZ = (mapDims.mapy + TILE_SIZE - 1) / TILE_SIZE;

	tileStride = tilesX;

	stateMap.clear();
	stateMap.resize(tilesX * tilesZ); // value-initialises: squares are zeroed

	RebuildExitOnlyBlocks();
}

void YardmapStatusEffectsMap::PostLoad() {
	tileStride = (mapDims.mapx + TILE_SIZE - 1) / TILE_SIZE;

	RebuildExitOnlyBlocks();
}