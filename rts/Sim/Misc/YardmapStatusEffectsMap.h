/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef YardmapStatusEffectsMap_H
#define YardmapStatusEffectsMap_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>

#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Objects/SolidObject.h"
#include "System/Log/ILog.h"

class YardmapStatusEffectsMap {
public:
	CR_DECLARE_STRUCT(YardmapStatusEffectsMap)

	enum SquareStates {
		EXIT_ONLY      = 0x01,
		BLOCK_BUILDING = 0x02,
	};

	const uint8_t& GetMapState(int x, int z) const {
		const int cx = std::clamp(x, 0, mapDims.mapxm1);
		const int cz = std::clamp(z, 0, mapDims.mapym1);
		const int tileId   = (cz / TILE_SIZE) * tileStride + (cx / TILE_SIZE);
		const int squareId = (cz & (TILE_SIZE - 1)) * TILE_SIZE  + (cx & (TILE_SIZE - 1));
		return stateMap[tileId].squares[squareId];
	}
	uint8_t& GetMapState(int x, int z) {
		return const_cast<uint8_t&>(std::as_const(*this).GetMapState(x, z));
	}

	bool AreAllFlagsSet(int x, int z, uint8_t flags) const { return (GetMapState(x, z) & flags) == flags; }
	bool AreAnyFlagsSet(int x, int z, uint8_t flags) const { return (GetMapState(x, z) & flags) != 0; }

	// Set/ClearFlags additionally maintain the coarse EXIT_ONLY presence grid
	// (see eoBlocks below), so RangeMayHaveExitOnly can reject a footprint scan
	// without touching the fine map.
	void SetFlags(int x, int z, uint8_t flags) {
		const int cx = std::clamp(x, 0, mapDims.mapxm1);
		const int cz = std::clamp(z, 0, mapDims.mapym1);
		uint8_t& st = GetMapState(cx, cz);
		if ((flags & EXIT_ONLY) && !(st & EXIT_ONLY))
			++eoBlocks[EOBlockIdx(cx, cz)];
		st |= flags;
	}
	void ClearFlags(int x, int z, uint8_t flags) {
		const int cx = std::clamp(x, 0, mapDims.mapxm1);
		const int cz = std::clamp(z, 0, mapDims.mapym1);
		uint8_t& st = GetMapState(cx, cz);
		if ((flags & EXIT_ONLY) && (st & EXIT_ONLY))
			--eoBlocks[EOBlockIdx(cx, cz)];
		st &= ~flags;
	}

	// True when some square in [xmin,xmax]x[zmin,zmax] may carry EXIT_ONLY.
	// Conservative by construction: a block counts > 0 whenever any square in
	// it is exit-only, so this never answers false while a real exit-only
	// square is in range — a caller that skips its scan on false therefore
	// returns exactly what the scan would have returned.
	bool RangeMayHaveExitOnly(int xmin, int xmax, int zmin, int zmax) const {
		const int bx0 = std::clamp(xmin, 0, mapDims.mapxm1) >> EO_BLOCK_SHIFT;
		const int bx1 = std::clamp(xmax, 0, mapDims.mapxm1) >> EO_BLOCK_SHIFT;
		const int bz0 = std::clamp(zmin, 0, mapDims.mapym1) >> EO_BLOCK_SHIFT;
		const int bz1 = std::clamp(zmax, 0, mapDims.mapym1) >> EO_BLOCK_SHIFT;
		for (int bz = bz0; bz <= bz1; ++bz) {
			const int row = bz * eoStride;
			for (int bx = bx0; bx <= bx1; ++bx)
				if (eoBlocks[row + bx] != 0) return true;
		}
		return false;
	}

	void ClearTile(int tileId);

	void InitNewYardmapStatusEffectsMap();

	void PostLoad();

private:
	// Each tile is exactly 64 bytes — one cache line.
	// Tiles are stored in row-major order: tileId = (z/8)*tileStride + (x/8).
	// Within a tile, squares are in row-major order: squareId = (z%8)*8 + (x%8).
	static constexpr int TILE_SIZE = 8;
	static constexpr int TILE_AREA = TILE_SIZE * TILE_SIZE; // 64

	struct alignas(64) Tile {
		CR_DECLARE_STRUCT(Tile)
		uint8_t squares[TILE_AREA];
	};
	static_assert(sizeof(Tile) == 64, "Tile must be exactly one cache line");
	// resize() value-initialises Tile, which zero-initialises squares only if
	// Tile remains a trivial aggregate — enforce that here.
	static_assert(std::is_trivially_default_constructible_v<Tile>,
	              "Tile must be trivially default constructible for resize() zero-init to hold");

	int tileStride = 0; // tiles per map row, set by Init / PostLoad

	// Coarse EXIT_ONLY presence grid: one counter per 16x16-square block,
	// holding how many squares in that block carry the flag.
	//
	// EXIT_ONLY is written in exactly one place (GroundBlockingObjectMap's
	// Add/RemoveGroundBlockingObject, for yardmaps declaring YARDMAP_EXITONLY),
	// so on a real map the flag exists only under a handful of factories while
	// CMoveMath::RangeHasExitOnly scans a whole unit footprint for it on every
	// collision query. The counter grid lets that scan be skipped wherever no
	// overlapping block holds any exit-only square at all, which is nearly
	// everywhere and nearly always.
	//
	// 1/256th of the map (a few KB, so permanently cache-resident) and still
	// finer than the footprints querying it. The shift also makes a status
	// tile (8x8, aligned) fall entirely inside one block, which is what lets
	// ClearTile correct the counter with a single subtraction.
	static constexpr int EO_BLOCK_SHIFT = 4;
	int EOBlockIdx(int cx, int cz) const {
		return (cz >> EO_BLOCK_SHIFT) * eoStride + (cx >> EO_BLOCK_SHIFT);
	}
	void RebuildExitOnlyBlocks();

	int eoStride = 0;
	std::vector<uint16_t> eoBlocks;

	typedef std::vector<Tile> TileMapType;
	TileMapType stateMap;
};

extern YardmapStatusEffectsMap yardmapStatusEffectsMap;

struct ObjectCollisionMapHelper {
	ObjectCollisionMapHelper() {}
	ObjectCollisionMapHelper(const CSolidObject& object) {}
	ObjectCollisionMapHelper(const MoveDef& moveDef, float ypos) {}
	ObjectCollisionMapHelper(const MoveDef& moveDef) {}

	bool IsExitOnlyAt(int x, int z) const {
		return yardmapStatusEffectsMap.AreAllFlagsSet(x, z, YardmapStatusEffectsMap::EXIT_ONLY);
	}
	void SetExitOnlyAt(int x, int z) {
		yardmapStatusEffectsMap.SetFlags(x, z, YardmapStatusEffectsMap::EXIT_ONLY);
	}
	void ClearExitOnlyAt(int x, int z) {
		yardmapStatusEffectsMap.ClearFlags(x, z, YardmapStatusEffectsMap::EXIT_ONLY);
	}

	bool IsBlockBuildingAt(int x, int z) const {
		return yardmapStatusEffectsMap.AreAllFlagsSet(x, z, YardmapStatusEffectsMap::BLOCK_BUILDING);
	}
	void SetBlockBuildingAt(int x, int z) {
		yardmapStatusEffectsMap.SetFlags(x, z, YardmapStatusEffectsMap::BLOCK_BUILDING);
	}
	void ClearBlockBuildingAt(int x, int z) {
		yardmapStatusEffectsMap.ClearFlags(x, z, YardmapStatusEffectsMap::BLOCK_BUILDING);
	}
};

#endif