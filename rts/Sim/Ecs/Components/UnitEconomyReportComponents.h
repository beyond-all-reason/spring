#ifndef UNIT_ECONOMY_REPORT_COMPONENTS_H__
#define UNIT_ECONOMY_REPORT_COMPONENTS_H__

#include "Sim/Misc/Resource.h"

namespace UnitEconomyReport {

struct SnapshotMake : public SResourcePack {
    using SResourcePack::operator=;
};

struct SnapshotUsage : public SResourcePack {
    using SResourcePack::operator=;
};

// struct SnapshotMetalUsage {
//     float value = 0.f;
// };

// struct SnapshotMetalMake {
//     float value = 0.f;
// };

// struct SnapshotEnergyUsage {
//     float value = 0.f;
// };

// struct SnapshotEnergyMake {
//     float value = 0.f;
// };

}

#endif