#ifndef UNIT_ECONOMY_UTILS_H__
#define UNIT_ECONOMY_UTILS_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
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

    static SResourcePack& GetCurrentMake(entt::entity entity) {
        auto comp = EcsMain::registry.try_get<UnitEconomy::ResourcesCurrentMake>(entity);
        if (comp == nullptr) {
            comp = &EcsMain::registry.emplace<UnitEconomy::ResourcesCurrentMake>(entity);
        }
        return *comp;
    }

    static SResourcePack& GetCurrentUsage(entt::entity entity) {
        auto comp = EcsMain::registry.try_get<UnitEconomy::ResourcesCurrentUsage>(entity);
        if (comp == nullptr) {
            comp = &EcsMain::registry.emplace<UnitEconomy::ResourcesCurrentUsage>(entity);
        }
        return *comp;
    }
};

#endif