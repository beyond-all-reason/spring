#include "UnitEconomyReportSystem.h"

#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyReportComponents.h"
#include "Sim/Misc/GlobalSynced.h"

UnitEconomyReportSystem unitEconomyReportSystem;

using namespace UnitEconomyReport;

void UnitEconomyReportSystem::Init()
{
}

void TakeEnergyMakeSnapshot(){
    auto group = EcsMain::registry.group<SnapshotEnergyMake>(entt::get<UnitEconomy::EnergyCurrentMake>);
    for (auto entity : group) {
        auto& displayValue = group.get<SnapshotEnergyMake>(entity).value;
        auto& counterValue = group.get<UnitEconomy::EnergyCurrentMake>(entity).value;

        displayValue = counterValue;
        counterValue = 0.f;
    }
}

void TakeMetalMakeSnapshot(){
    auto group = EcsMain::registry.group<SnapshotMetalMake>(entt::get<UnitEconomy::MetalCurrentMake>);
    for (auto entity : group) {
        auto& displayValue = group.get<SnapshotMetalMake>(entity).value;
        auto& counterValue = group.get<UnitEconomy::MetalCurrentMake>(entity).value;

        displayValue = counterValue;
        counterValue = 0.f;
    }
}

void TakeEnergyUseSnapshot(){
    auto group = EcsMain::registry.group<SnapshotEnergyUsage>(entt::get<UnitEconomy::EnergyCurrentUsage>);
    for (auto entity : group) {
        auto& displayValue = group.get<SnapshotEnergyUsage>(entity).value;
        auto& counterValue = group.get<UnitEconomy::EnergyCurrentUsage>(entity).value;

        displayValue = counterValue;
        counterValue = 0.f;
    }
}

void TakeMetalUseSnapshot(){
    auto group = EcsMain::registry.group<SnapshotMetalUsage>(entt::get<UnitEconomy::MetalCurrentUsage>);
    for (auto entity : group) {
        auto& displayValue = group.get<SnapshotMetalUsage>(entity).value;
        auto& counterValue = group.get<UnitEconomy::MetalCurrentUsage>(entity).value;

        displayValue = counterValue;
        counterValue = 0.f;
    }
}

void UnitEconomyReportSystem::Update() {
    if ((gs->frameNum % UNIT_ECONOMY_REPORT_UPDATE_RATE) == UNIT_ECONOMY_REPORT_TICK) {
        TakeEnergyMakeSnapshot();
        TakeMetalMakeSnapshot();
        TakeEnergyUseSnapshot();
        TakeMetalUseSnapshot();
    }
}
