#include "UnitEconomyReportSystem.h"

#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/SystemGlobalComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyReportComponents.h"
#include "Sim/Ecs/Utils/SystemGlobalUtils.h"
#include "Sim/Misc/GlobalSynced.h"

#include "System/TimeProfiler.h"


using namespace SystemGlobals;
using namespace UnitEconomyReport;

template<class T>
void AddComponent(entt::registry &registry, entt::entity entity) {
    if (! registry.all_of<T>(entity))
        registry.emplace<T>(entity);
}

template<class T>
void RemoveComponent(entt::registry &registry, entt::entity entity) {
    registry.remove<T>(entity);
}

template<class T>
void AddComponentToOwnerOfEcoTask(entt::registry &registry, entt::entity taskEntity) {
    auto OwnerComp = EcsMain::registry.try_get<Units::OwningEntity>(taskEntity);
    if (OwnerComp == nullptr) return;

    auto entity = OwnerComp->value;
    if (! registry.all_of<T>(entity))
        registry.emplace<T>(entity);
}

void SetupObserversForEcoTasks() {
    EcsMain::registry.on_construct<FlowEconomy::ResourceAdd>()
                     .connect<&AddComponentToOwnerOfEcoTask<UnitEconomy::ResourcesCurrentMake>>();
    EcsMain::registry.on_construct<FlowEconomy::ResourceUse>()
                     .connect<&AddComponentToOwnerOfEcoTask<UnitEconomy::ResourcesCurrentUsage>>();
}

void SetupObserversForResourceTracking() {
    EcsMain::registry.on_construct<UnitEconomy::ResourcesCurrentMake>()
                     .connect<&AddComponent<UnitEconomyReport::SnapshotMake>>();
    EcsMain::registry.on_construct<UnitEconomy::ResourcesCurrentUsage>()
                     .connect<&AddComponent<UnitEconomyReport::SnapshotUsage>>();

    EcsMain::registry.on_destroy<UnitEconomy::ResourcesCurrentMake>()
                     .connect<&RemoveComponent<UnitEconomyReport::SnapshotMake>>();
    EcsMain::registry.on_destroy<UnitEconomy::ResourcesCurrentUsage>()
                     .connect<&RemoveComponent<UnitEconomyReport::SnapshotUsage>>();
}

void UnitEconomyReportSystem::Init()
{
    systemGlobals.CreateSystemComponent<UnitEconomyReportSystemComponent>();

    SetupObserversForEcoTasks();
    SetupObserversForResourceTracking();
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

void TakeMakeSnapshot(UnitEconomyReportSystemComponent& system){
    auto group = EcsMain::registry.group<SnapshotMake>(entt::get<UnitEconomy::ResourcesCurrentMake>);
    for (auto entity : group) {
        auto& displayValue = group.get<SnapshotMake>(entity);
        auto& counterValue = group.get<UnitEconomy::ResourcesCurrentMake>(entity);

        displayValue.resources[system.activeBuffer] = counterValue;
        counterValue = SResourcePack();

        // Example of Patch/Replace approach - needed if observers are used
        // EcsMain::registry.patch<SnapshotMake>(entity, [entity, &system, &group](auto& snapshot){
        //         const auto& counterValue = group.get<UnitEconomy::ResourcesCurrentMake>(entity);
        //         snapshot.resources[system.activeBuffer] = counterValue;
        //     });
        // EcsMain::registry.replace<UnitEconomy::ResourcesCurrentMake>(entity, SResourcePack());
    }
}

void TakeUseSnapshot(UnitEconomyReportSystemComponent& system){
    auto group = EcsMain::registry.group<SnapshotUsage>(entt::get<UnitEconomy::ResourcesCurrentUsage>);
    for (auto entity : group) {
        auto& displayValue = group.get<SnapshotUsage>(entity);
        auto& counterValue = group.get<UnitEconomy::ResourcesCurrentUsage>(entity);

        displayValue.resources[system.activeBuffer] = counterValue;
        counterValue = SResourcePack();
        //LOG("%s: energy snapshot is %f", __func__, displayValue);
    }
}

void UnitEconomyReportSystem::Update() {
    if ((gs->frameNum % UNIT_ECONOMY_REPORT_UPDATE_RATE) != UNIT_ECONOMY_REPORT_TICK)
        return;

    LOG("UnitEconomyReportSystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::UnitEconomySystem::Update");

    auto& system = systemGlobals.GetSystemComponent<UnitEconomyReportSystemComponent>();
    system.activeBuffer = (system.activeBuffer + 1) % SnapshotBase::BUFFERS;

    TakeMakeSnapshot(system);
    TakeUseSnapshot(system);
}
