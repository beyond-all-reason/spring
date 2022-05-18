#ifndef UNIT_ECONOMY_COMPONENTS_H__
#define UNIT_ECONOMY_COMPONENTS_H__

#include "Sim/Misc/Resource.h"

namespace UnitEconomy {

struct ResourcesCurrentMake : public SResourcePack {
    using SResourcePack::operator=;
};

struct ResourcesCurrentUsage : public SResourcePack {
    using SResourcePack::operator=;
};

// struct MetalCurrentUsage {
//     float value = 0.f;
// };

// struct MetalCurrentMake {
//     float value = 0.f;
// };

// struct EnergyCurrentUsage {
//     float value = 0.f;
// };

// struct EnergyCurrentMake {
//     float value = 0.f;
// };

}

#endif