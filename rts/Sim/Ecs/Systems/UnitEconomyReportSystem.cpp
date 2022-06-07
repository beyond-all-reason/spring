#include "UnitEconomyReportSystem.h"

#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyReportComponents.h"
#include "Sim/Misc/GlobalSynced.h"

#include "System/TimeProfiler.h"


UnitEconomyReportSystem unitEconomyReportSystem;

using namespace UnitEconomyReport;

void UnitEconomyReportSystem::Init()
{
    updatesPerSecond = GAME_SPEED / UNIT_ECONOMY_REPORT_UPDATE_RATE;
}

// Should work but GCC 10.3 cannot process this correctly
// template<class SnapshotType, class ResourceCounterType>
// void TakeSnapshot(){
//     auto group = EcsMain::registry.group<SnapshotType>(entt::get<ResourceCounterType>);
//     for (auto entity : group) {
//         auto& displayValue = group.get<SnapshotType>(entity).value;
//         auto& counterValue = group.get<ResourceCounterType>(entity).value;

//         displayValue = counterValue;
//         counterValue = 0.f;
//     }
// }

void UnitEconomyReportSystem::TakeMakeSnapshot(){
    auto group = EcsMain::registry.group<SnapshotMake>(entt::get<UnitEconomy::ResourcesCurrentMake>);
    for (auto entity : group) {
        auto& displayValue = group.get<SnapshotMake>(entity);
        auto& counterValue = group.get<UnitEconomy::ResourcesCurrentMake>(entity);

        displayValue = counterValue * updatesPerSecond;
        counterValue = SResourcePack();
    }
}

void UnitEconomyReportSystem::TakeUseSnapshot(){
    auto group = EcsMain::registry.group<SnapshotUsage>(entt::get<UnitEconomy::ResourcesCurrentUsage>);
    for (auto entity : group) {
        auto& displayValue = group.get<SnapshotUsage>(entity);
        auto& counterValue = group.get<UnitEconomy::ResourcesCurrentUsage>(entity);

        displayValue = counterValue * updatesPerSecond;
        counterValue = SResourcePack();
        //LOG("%s: energy snapshot is %f", __func__, displayValue);
    }
}

void UnitEconomyReportSystem::Update() {
    if ((gs->frameNum % UNIT_ECONOMY_REPORT_UPDATE_RATE) != UNIT_ECONOMY_REPORT_TICK)
        return;

    LOG("UnitEconomyReportSystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::UnitEconomySystem::Update");

    TakeMakeSnapshot();
    TakeUseSnapshot();
}
