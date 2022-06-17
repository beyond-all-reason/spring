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
        LOG("%s: (%f,%f)", __func__, snapshot[0], snapshot[1]);
        return snapshot;
    }

    static SResourcePack GetCurrentResourceMakeSnapshot(entt::entity entity) {
        SResourcePack snapshot;
        auto comp = EcsMain::registry.try_get<UnitEconomyReport::SnapshotMake>(entity);
        if (comp != nullptr) {
            for (int i=0; i<UnitEconomyReport::SnapshotUsage::BUFFERS; ++i)
                snapshot += comp->resources[i];
        }
        LOG("%s: (%f,%f)", __func__, snapshot[0], snapshot[1]);
        return snapshot;
    }

    static SResourcePack& GetCurrentMake(entt::entity entity) {
        auto snapshotMissing = ( EcsMain::registry.try_get<UnitEconomyReport::SnapshotMake>(entity) == nullptr );
        if (snapshotMissing) EcsMain::registry.emplace<UnitEconomyReport::SnapshotMake>(entity);
        return EcsMain::registry.get_or_emplace<UnitEconomy::ResourcesCurrentMake>(entity);
    }

    static SResourcePack& GetCurrentUsage(entt::entity entity) {
        auto snapshotMissing = ( EcsMain::registry.try_get<UnitEconomyReport::SnapshotUsage>(entity) == nullptr );
        if (snapshotMissing) EcsMain::registry.emplace<UnitEconomyReport::SnapshotUsage>(entity);
        return EcsMain::registry.get_or_emplace<UnitEconomy::ResourcesCurrentUsage>(entity);
    }

    static void SetupEconomyTacking(entt::entity entity) {
        AddComponentsIfNotExist
            < UnitEconomy::ResourcesCurrentMake
            , UnitEconomy::ResourcesCurrentUsage
            , UnitEconomyReport::SnapshotMake
            , UnitEconomyReport::SnapshotUsage
            >(entity);
    }
};

#endif