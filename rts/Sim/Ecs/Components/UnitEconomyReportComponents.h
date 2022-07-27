#ifndef UNIT_ECONOMY_REPORT_COMPONENTS_H__
#define UNIT_ECONOMY_REPORT_COMPONENTS_H__

#include "Sim/Misc/Resource.h"

namespace UnitEconomyReport {

struct SnapshotBase {
    constexpr static int BUFFERS = 2;

    SResourcePack resources[BUFFERS];
};

template<class Archive>
void serialize(Archive &ar, SnapshotBase &c) { ar(c.resources); }


struct SnapshotMake : public SnapshotBase {
};

struct SnapshotUsage : public SnapshotBase {
};

template<class Archive, class Snapshot>
void serializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component
        < SnapshotMake, SnapshotUsage
        >(archive);
}

}

#endif