#include "UnitEconomySystem.h"

#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Ecs/Components/BuildComponents.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/TimeProfiler.h"

UnitEconomySystem unitEconomySystem;

using namespace UnitEconomy;

void UnitEconomySystem::Init()
{
    economyMultiplier = ((float)UNIT_ECONOMY_UPDATE_RATE) / ((float)GAME_SPEED);
}

// static float GetTeamProrationRate(int teamId, Build::ProrationRate prorationType){
//     auto team = teamHandler.Team(teamId);
//     return team->prorationRates[(int)prorationType];
// }

// These really should be templated, but GCC 10.3 can't compile the templated version.
void UnitEconomySystem::UpdateEnergyIncomeTracking(){
    // auto group = EcsMain::registry.group<EnergyCurrentMake>(entt::get<Units::Team>);
    // for (auto entity : group) {
    //     auto& ecoTrack = group.get<EnergyCurrentMake>(entity).value;
    //     auto buildRate = GetTeamProrationRate(group.get<Units::Team>(entity).value, Build::ProrationRate::PRORATION_ONLY_METAL);

    //     ecoTrack += GetOptionalComponent<FlowEconomy::EnergyFixedIncome>(entity, 0.f) * economyMultiplier;
    //     ecoTrack += GetOptionalComponent<FlowEconomy::EnergyProratableIncome>(entity, 0.f) * buildRate * economyMultiplier;
    // }
// {
//     auto group = EcsMain::registry.group<FlowEconomy::EnergyProratableIncome>(entt::get<Units::Team, Units::OwningEntity>);
//     for (auto entity : group) {
//         auto income = (group.get<FlowEconomy::EnergyProratableIncome>(entity)).value;
//         auto teamId = (group.get<Units::Team>(entity)).value;
//         auto owner = (group.get<Units::OwningEntity>(entity)).value;

//         auto ecoTrack = EcsMain::registry.try_get<EnergyCurrentMake>(owner);
//         if (ecoTrack != nullptr) {
//             auto buildRate = GetTeamProrationRate(group.get<Units::Team>(entity).value, Build::ProrationRate::PRORATION_ONLY_METAL);
//             ecoTrack->value += income * buildRate * economyMultiplier;
//         }
//     }
// }
// {
//     auto group = EcsMain::registry.group<FlowEconomy::EnergyFixedIncome>(entt::get<Units::OwningEntity>);
//     for (auto entity : group) {
//         auto income = (group.get<FlowEconomy::EnergyFixedIncome>(entity)).value;
//         auto owner = (group.get<Units::OwningEntity>(entity)).value;

//         auto ecoTrack = EcsMain::registry.try_get<EnergyCurrentMake>(owner);
//         if (ecoTrack != nullptr) {
//             ecoTrack->value += income * economyMultiplier;
//         }
//     }
// }
}

void UnitEconomySystem::UpdateMetalIncomeTracking(){
    // auto group = EcsMain::registry.group<MetalCurrentMake>(entt::get<Units::Team>);
    // for (auto entity : group) {
    //     auto& ecoTrack = group.get<MetalCurrentMake>(entity).value;
    //     auto buildRate = GetTeamProrationRate(group.get<Units::Team>(entity).value, Build::ProrationRate::PRORATION_ONLY_ENERGY);

    //     ecoTrack += GetOptionalComponent<FlowEconomy::MetalFixedIncome>(entity, 0.f) * economyMultiplier;
    //     ecoTrack += GetOptionalComponent<FlowEconomy::MetalProratableIncome>(entity, 0.f) * buildRate * economyMultiplier;
    // }
// {
//     auto group = EcsMain::registry.group<FlowEconomy::MetalProratableIncome>(entt::get<Units::Team, Units::OwningEntity>);
//     for (auto entity : group) {
//         auto income = (group.get<FlowEconomy::MetalProratableIncome>(entity)).value;
//         auto teamId = (group.get<Units::Team>(entity)).value;
//         auto owner = (group.get<Units::OwningEntity>(entity)).value;

//         auto ecoTrack = EcsMain::registry.try_get<MetalCurrentMake>(owner);
//         if (ecoTrack != nullptr) {
//             auto buildRate = GetTeamProrationRate(group.get<Units::Team>(entity).value, Build::ProrationRate::PRORATION_ONLY_METAL);
//             ecoTrack->value += income * buildRate * economyMultiplier;
//         }
//     }
// }
// {
//     auto group = EcsMain::registry.group<FlowEconomy::MetalFixedIncome>(entt::get<Units::OwningEntity>);
//     for (auto entity : group) {
//         auto income = (group.get<FlowEconomy::MetalFixedIncome>(entity)).value;
//         auto owner = (group.get<Units::OwningEntity>(entity)).value;

//         auto ecoTrack = EcsMain::registry.try_get<MetalCurrentMake>(owner);
//         if (ecoTrack != nullptr) {
//             ecoTrack->value += income * economyMultiplier;
//         }
//     }
// }
}

void UnitEconomySystem::UpdateEnergyUsageTracking(){
    // auto combinedGroup = EcsMain::registry.group<EnergyCurrentUsage, MetalCurrentUsage>(entt::get<Units::Team>);
    // auto group = EcsMain::registry.group<EnergyCurrentUsage>(entt::get<Units::Team>);
    // auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    // for (auto entity : group) {
    //     if (entitiesLeftToProcess-- == 0) break;
    //     auto& ecoTrack = group.get<EnergyCurrentUsage>(entity).value;
    //     auto buildRate = GetTeamProrationRate(group.get<Units::Team>(entity).value, Build::ProrationRate::PRORATION_ONLY_ENERGY);

    //     ecoTrack += GetOptionalComponent<FlowEconomy::EnergyFixedUse>(entity, 0.f) * economyMultiplier;
    //     ecoTrack += GetOptionalComponent<FlowEconomy::EnergyProratableUse>(entity, 0.f) * buildRate * economyMultiplier;
    // }
// {
//     auto group = EcsMain::registry.group<FlowEconomy::EnergyProratableUse>(entt::get<Units::Team, Units::OwningEntity>);
//     for (auto entity : group) {
//         auto income = (group.get<FlowEconomy::EnergyProratableUse>(entity)).value;
//         auto teamId = (group.get<Units::Team>(entity)).value;
//         auto owner = (group.get<Units::OwningEntity>(entity)).value;

//         auto ecoTrack = EcsMain::registry.try_get<EnergyCurrentUsage>(owner);
//         if (ecoTrack != nullptr) {
//             auto buildRate = GetTeamProrationRate(group.get<Units::Team>(entity).value, Build::ProrationRate::PRORATION_ONLY_ENERGY);
//             ecoTrack->value += income * buildRate * economyMultiplier;
//         }
//     }
// }
}

void UnitEconomySystem::UpdateMetalUsageTracking(){
    // auto combinedGroup = EcsMain::registry.group<EnergyCurrentUsage, MetalCurrentUsage>(entt::get<Units::Team>);
    // auto group = EcsMain::registry.group<MetalCurrentUsage>(entt::get<Units::Team>);
    // auto entitiesLeftToProcess = group.size() - combinedGroup.size();
    // for (auto entity : group) {
    //     if (entitiesLeftToProcess-- == 0) break;
    //     auto& ecoTrack = group.get<MetalCurrentUsage>(entity).value;
    //     auto buildRate = GetTeamProrationRate(group.get<Units::Team>(entity).value, Build::ProrationRate::PRORATION_ONLY_METAL);

    //     ecoTrack += GetOptionalComponent<FlowEconomy::MetalProratableUse>(entity, 0.f) * buildRate * economyMultiplier;
    // }
// {
//     auto group = EcsMain::registry.group<FlowEconomy::MetalProratableUse>(entt::get<Units::Team, Units::OwningEntity>);
//     for (auto entity : group) {
//         auto income = (group.get<FlowEconomy::MetalProratableUse>(entity)).value;
//         auto teamId = (group.get<Units::Team>(entity)).value;
//         auto owner = (group.get<Units::OwningEntity>(entity)).value;

//         auto ecoTrack = EcsMain::registry.try_get<MetalCurrentUsage>(owner);
//         if (ecoTrack != nullptr) {
//             auto buildRate = GetTeamProrationRate(group.get<Units::Team>(entity).value, Build::ProrationRate::PRORATION_ONLY_METAL);
//             ecoTrack->value += income * buildRate * economyMultiplier;
//         }
//     }
// }
}

void UnitEconomySystem::UpdateEconomyCombinedUsageTracking(){
    // auto group = EcsMain::registry.group<EnergyCurrentUsage, MetalCurrentUsage>(entt::get<Units::Team>);
    // for (auto entity : group) {
    //     auto& energyTrack = group.get<EnergyCurrentUsage>(entity).value;
    //     auto& metalTrack = group.get<MetalCurrentUsage>(entity).value;
    //     auto buildRate = GetTeamProrationRate(group.get<Units::Team>(entity).value, Build::ProrationRate::PRORATION_ALL);

    //     energyTrack += GetOptionalComponent<FlowEconomy::EnergyFixedUse>(entity, 0.f) * economyMultiplier;
    //     energyTrack += GetOptionalComponent<FlowEconomy::EnergyProratableUse>(entity, 0.f) * buildRate * economyMultiplier;
    //     metalTrack += GetOptionalComponent<FlowEconomy::MetalProratableUse>(entity, 0.f) * buildRate * economyMultiplier;
    // }
}

void UnitEconomySystem::Update() {
    if ((gs->frameNum % UNIT_ECONOMY_UPDATE_RATE) != UNIT_ECONOMY_TICK)
        return;

    LOG("UnitEconomySystem::%s: %d", __func__, gs->frameNum);

    SCOPED_TIMER("ECS::UnitEconomySystem::Update");

    // UpdateEnergyIncomeTracking();
    // UpdateMetalIncomeTracking();
    // UpdateEnergyUsageTracking();
    // UpdateMetalUsageTracking();
    //UpdateEconomyCombinedUsageTracking();
}
