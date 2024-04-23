/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXIT_ONLY_MAP_H
#define EXIT_ONLY_MAP_H

// #include <array>
#include <cstdint>
#include <vector>

#include "System/Log/ILog.h"

class ExitOnlyMap {
public:
    static constexpr int resolution = 2;
    static constexpr int sectorWidth = 16;
    static constexpr int internalSectorWidth = sectorWidth / resolution;

    uint32_t interleave(uint32_t x0, uint32_t y0)
    {
        static const uint32_t B[] = {0x0000FFFF, 0x00FF00FF, 0x0F0F0F0F, 0x33333333, 0x55555555};
        static const unsigned S[] = {16, 8, 4, 2, 1};

        uint32_t x = x0;
        uint32_t y = y0;

        for(unsigned i = 0; i < sizeof(B)/sizeof(B[0]); i++)
        {
            x = (x | (x << S[i])) & B[i];
            y = (y | (y << S[i])) & B[i];
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