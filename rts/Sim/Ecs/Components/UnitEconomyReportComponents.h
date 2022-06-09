#ifndef UNIT_ECONOMY_REPORT_COMPONENTS_H__
#define UNIT_ECONOMY_REPORT_COMPONENTS_H__

#include "Sim/Misc/Resource.h"

namespace UnitEconomyReport {

struct SnapshotBase {
    constexpr static int BUFFERS = 2;

    SResourcePack resources[BUFFERS];
};

struct SnapshotMake : public SnapshotBase {
};

struct SnapshotUsage : public SnapshotBase {
};

}

#endif