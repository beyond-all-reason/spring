/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXIT_ONLY_MAP_H
#define EXIT_ONLY_MAP_H

#include <array>
#include <cstdint>
#include <vector>

#include "Sim/Misc/GlobalConstants.h"
#include "System/Log/ILog.h"

class ExitOnlyMap {

    static constexpr std::array<uint32_t, 5> zMasks = {0x0000FFFF, 0x00FF00FF, 0x0F0F0F0F, 0x33333333, 0x55555555};
    static constexpr std::array<uint32_t, 5> zShifts = {16, 8, 4, 2, 1};

public:
    static constexpr int resolution = SPRING_FOOTPRINT_SCALE;

    uint32_t interleave(uint32_t x0, uint32_t y0)
    {
        uint32_t x = x0;
        uint32_t y = y0;

        for(uint32_t i = 0; i < zMasks.size(); i++)
        {
            x = (x | (x << zShifts[i])) & zMasks[i];
            y = (y | (y << zShifts[i])) & zMasks[i];
        }
        return x | (y << 1);
    }

    uint8_t& GetExitOnlyState(int x, int z) { return stateMap[interleave(x / resolution, z / resolution)]; }
    uint8_t& GetExitOnlyStateNative(int x, int z) { return stateMap[interleave(x, z)]; }

    bool IsExitOnly(int x, int z) { return GetExitOnlyState(x, z); }
    bool IsExitOnlyNative(int x, int z) { return GetExitOnlyStateNative(x, z); }
    void SetExitOnly(int x, int z) { GetExitOnlyState(x, z) = true; }
    void ClearExitOnly(int x, int z) { GetExitOnlyState(x, z) = false; }

    void InitNewExitOnlyMap();

    typedef std::vector<uint8_t> ExitOnlyMapType;
    ExitOnlyMapType stateMap;
};

extern ExitOnlyMap exitOnlyMap;

#endif