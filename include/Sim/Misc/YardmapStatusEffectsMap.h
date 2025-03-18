/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef YardmapStatusEffectsMap_H
#define YardmapStatusEffectsMap_H

#include <cstdint>
#include <vector>

#include "Map/ReadMap.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Objects/SolidObject.h"
#include "System/Log/ILog.h"

class YardmapStatusEffectsMap {
public:
	CR_DECLARE(YardmapStatusEffectsMap)

    enum SquareStates {
        EXIT_ONLY      = 0x01,
        BLOCK_BUILDING = 0x02,
    };

    uint32_t interleave(uint32_t x, uint32_t z)
    {
		x = std::clamp(int(x), 0, mapDims.mapxm1);
		z = std::clamp(int(z), 0, mapDims.mapym1);

		static constexpr uint32_t zMasks[] = {0x0000FFFF, 0x00FF00FF, 0x0F0F0F0F, 0x33333333, 0x55555555};
		static constexpr uint32_t zShifts[] = {16, 8, 4, 2, 1};

        for(uint32_t i = 0; i < sizeof(zMasks)/sizeof(uint32_t); i++)
        {
            x = (x | (x << zShifts[i])) & zMasks[i];
            z = (z | (z << zShifts[i])) & zMasks[i];
        }
        return x | (z << 1);
    }

    uint8_t& GetMapState(int x, int z) { return stateMap[interleave(x, z)]; }

    bool AreAllFlagsSet(int x, int z, uint8_t flags) { return (GetMapState(x, z) & flags) == flags; }
	bool AreAnyFlagsSet(int x, int z, uint8_t flags) { return (GetMapState(x, z) & flags) != 0; }
	
    void SetFlags(int x, int z, uint8_t flags) { GetMapState(x, z) |= flags; }
    void ClearFlags(int x, int z, uint8_t flags) { GetMapState(x, z) &= ~flags; }

    void InitNewYardmapStatusEffectsMap();

    typedef std::vector<uint8_t> ExitOnlyMapType;
    ExitOnlyMapType stateMap;
};

extern YardmapStatusEffectsMap yardmapStatusEffectsMap;

struct ObjectCollisionMapHelper {
	bool collidesUnderWater = false;
	bool collidesAboveWater = false;

	ObjectCollisionMapHelper() {}

	ObjectCollisionMapHelper(const CSolidObject& object) {
		// SetObjectCollisionStates(object);
	}

	ObjectCollisionMapHelper(const MoveDef& moveDef, float ypos) {
		// SetMoveDefCollisionStates(moveDef, ypos);
	}

	ObjectCollisionMapHelper(const MoveDef& moveDef) {
		// SetMoveDefCollisionStates(moveDef);
	}

	// float GetMoveCollisionHeight(const CSolidObject& object) const {
	// 	if (object.moveDef != nullptr)
	// 		return object.moveDef->height;
	// 	else
	// 		return object.height;
	// }

	// bool IsOnWaterSurface(const CSolidObject& object) const {
	// 	if (object.moveDef != nullptr)
	// 		return !object.moveDef->isSubmersible;
	// 	else {
	// 		auto unit = dynamic_cast<const CUnit*>(&object);
	// 		if (unit != nullptr)
	// 			return unit->FloatOnWater();
	// 	}
	// 	return false;
	// }

	// void SetObjectCollisionStates(const CSolidObject& object) {
	// 	const bool floatsOnWater = IsOnWaterSurface(object);
	// 	collidesUnderWater = !floatsOnWater;
	// 	collidesAboveWater = (floatsOnWater || object.pos.y + GetMoveCollisionHeight(object) >= 0.f);
	// }

    // void SetMoveDefCollisionStates(const MoveDef& moveDef, float ypos) {
	// 	const bool floatsOnWater = !moveDef.isSubmersible;
	// 	collidesUnderWater = !floatsOnWater;
	// 	collidesAboveWater = (floatsOnWater || ypos + moveDef.height >= 0.f);
    // }

    // void SetMoveDefCollisionStates(const MoveDef& moveDef) {
	// 	collidesUnderWater = moveDef.isSubmersible;
	// 	collidesAboveWater = true;
    // }

	uint8_t GetExitOnlyFlags() const {
		return YardmapStatusEffectsMap::EXIT_ONLY;
	}

	bool IsExitOnlyAt(int x, int z) const {
		return yardmapStatusEffectsMap.AreAllFlagsSet(x, z, GetExitOnlyFlags());
	}

	void SetExitOnlyAt(int x, int z) const {
		yardmapStatusEffectsMap.SetFlags(x, z, GetExitOnlyFlags());
	}

	void ClearExitOnlyAt(int x, int z) const {
		yardmapStatusEffectsMap.ClearFlags(x, z, GetExitOnlyFlags());
	}

	uint8_t GetBlockBuildingFlags() const {
		return YardmapStatusEffectsMap::BLOCK_BUILDING;
	}

    bool IsBlockBuildingAt(int x, int z) const {
		return yardmapStatusEffectsMap.AreAllFlagsSet(x, z, GetBlockBuildingFlags());
	}

	void SetBlockBuildingAt(int x, int z) const {
		yardmapStatusEffectsMap.SetFlags(x, z, GetBlockBuildingFlags());
	}

	void ClearBlockBuildingAt(int x, int z) const {
		yardmapStatusEffectsMap.ClearFlags(x, z, GetBlockBuildingFlags());
	}
};

#endif