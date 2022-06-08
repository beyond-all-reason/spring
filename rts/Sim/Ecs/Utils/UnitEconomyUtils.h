#ifndef UNIT_ECONOMY_UTILS_H__
#define UNIT_ECONOMY_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/UnitEconomyReportComponents.h"
#include "Sim/Misc/Resource.h"

class UnitEconomyUtils {

public:
    static SResourcePack GetCurrentResourceUsageSnapshot(entt::entity entity) {
        SResourcePack snapshot;
        auto comp = EcsMain::registry.try_get<UnitEconomyReport::SnapshotUsage>(entity);
        if (comp != nullptr) {
            for (int i=0; i<UnitEconomyReport::SnapshotUsage::BUFFERS; ++i)
                snapshot += comp->resources[i];
        }
        return snapshot;
    }

    static SResourcePack GetCurrentResourceMakeSnapshot(entt::entity entity) {
        SResourcePack snapshot;
        auto comp = EcsMain::registry.try_get<UnitEconomyReport::SnapshotMake>(entity);
        if (comp != nullptr) {
            for (int i=0; i<UnitEconomyReport::SnapshotUsage::BUFFERS; ++i)
                snapshot += comp->resources[i];
        }
        return snapshot;
    }
};

#endif