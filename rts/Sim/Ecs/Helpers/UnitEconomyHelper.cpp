#include "UnitEconomyHelper.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyReportComponents.h"
#include "Sim/Units/Unit.h"

template<class T>
void UpdateUnitEconomy(entt::entity entity, float amount){
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    if (amount <= 0.f){
        EcsMain::registry.remove<T>(entity);
        LOG("%s: %f [REMOVED]", __func__, amount);
    }
    else {
        EcsMain::registry.emplace_or_replace<T>(entity, amount);
        LOG("%s: %f", __func__, amount);
    }
}

void UnitEconomyHelper::UpdateUnitProratableEnergyIncome(CUnit *unit, float amount){
    UpdateUnitEconomy<EnergyProratableIncome>(unit->entityReference, amount);
}

void UnitEconomyHelper::UpdateUnitProratableMetalIncome(CUnit *unit, float amount){
    UpdateUnitEconomy<MetalProratableIncome>(unit->entityReference, amount);
}

void UnitEconomyHelper::UpdateUnitFixedEnergyIncome(CUnit *unit, float amount){
    UpdateUnitEconomy<EnergyFixedIncome>(unit->entityReference, amount);
}

// void FlowEconomySystem::UpdateUnitFixedEnergyIncome(entt::entity entity, float amount) {
//     UpdateUnitEconomy<EnergyFixedIncome>(entity, amount);
// }

void UnitEconomyHelper::UpdateUnitFixedMetalIncome(CUnit *unit, float amount) {
    UpdateUnitEconomy<MetalFixedIncome>(unit->entityReference, amount);
}

void UnitEconomyHelper::UpdateUnitFixedEnergyExpense(CUnit *unit, float amount){
    UpdateUnitEconomy<EnergyFixedUse>(unit->entityReference, amount);
}

void UnitEconomyHelper::UpdateUnitFixedMetalExpense(CUnit *unit, float amount) {
    UpdateUnitEconomy<MetalFixedUse>(unit->entityReference, amount);
}

void UnitEconomyHelper::UpdateUnitProratableEnergyUse(CUnit *unit, float amount){
    UpdateUnitEconomy<EnergyProratableUse>(unit->entityReference, amount);
}

void UnitEconomyHelper::UpdateUnitProratableMetalUse(CUnit *unit, float amount){
    //UpdateUnitEconomy<MetalProratableUse>(unit->entityReference, amount);
    auto entity = unit->entityReference;
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    if (amount <= 0.f){
        EcsMain::registry.remove<FlowEconomy::MetalProratableUse>(entity);
        LOG("%s: %f [REMOVED]", __func__, amount);

        if (!EcsMain::registry.all_of<FlowEconomy::MetalFixedUse>(entity)){
            EcsMain::registry.remove<UnitEconomy::MetalCurrentMake>(entity);
            EcsMain::registry.remove<UnitEconomyReport::SnapshotMetalMake>(entity);
        }
    }
    else {
        EcsMain::registry.emplace_or_replace<FlowEconomy::MetalProratableUse>(entity, amount);
        if (!EcsMain::registry.all_of<UnitEconomy::MetalCurrentMake>(entity)){
            EcsMain::registry.emplace<UnitEconomy::MetalCurrentMake>(entity);
        }
        if (!EcsMain::registry.all_of<UnitEconomyReport::SnapshotMetalMake>(entity)){
            EcsMain::registry.emplace<UnitEconomyReport::SnapshotMetalMake>(entity);
        }
        LOG("%s: %f", __func__, amount);
    }
}

void UnitEconomyHelper::UpdateEconomyTrackEnergyMake(entt::entity entity){
    if (EcsMain::registry.any_of<FlowEconomy::EnergyFixedIncome, FlowEconomy::EnergyProratableIncome>(entity)){
        // check add function
        if (!EcsMain::registry.all_of<UnitEconomy::EnergyCurrentMake>(entity)){
            EcsMain::registry.emplace<UnitEconomy::EnergyCurrentMake>(entity);
        }
        if (!EcsMain::registry.all_of<UnitEconomyReport::SnapshotEnergyMake>(entity)){
            EcsMain::registry.emplace<UnitEconomyReport::SnapshotEnergyMake>(entity);
        }
        // check add function
    }
    else {
        EcsMain::registry.remove<UnitEconomy::EnergyCurrentMake>(entity);
        EcsMain::registry.remove<UnitEconomyReport::SnapshotEnergyMake>(entity);
    }
}

void UnitEconomyHelper::UpdateEconomyTrackEnergyUse(entt::entity entity){
    if (EcsMain::registry.any_of<FlowEconomy::EnergyFixedUse, FlowEconomy::EnergyProratableUse>(entity)){
        if (!EcsMain::registry.all_of<UnitEconomy::EnergyCurrentUsage>(entity)){
            EcsMain::registry.emplace<UnitEconomy::EnergyCurrentUsage>(entity);
        }
        if (!EcsMain::registry.all_of<UnitEconomyReport::SnapshotEnergyUsage>(entity)){
            EcsMain::registry.emplace<UnitEconomyReport::SnapshotEnergyUsage>(entity);
        }
    }
    else {
        EcsMain::registry.remove<UnitEconomy::EnergyCurrentUsage>(entity);
        EcsMain::registry.remove<UnitEconomyReport::SnapshotEnergyUsage>(entity);
    }
}

void UnitEconomyHelper::UpdateEconomyTrackMetalMake(entt::entity entity){
    if (EcsMain::registry.any_of<FlowEconomy::MetalFixedIncome, FlowEconomy::MetalProratableIncome>(entity)){
        if (!EcsMain::registry.all_of<UnitEconomy::MetalCurrentMake>(entity)){
            EcsMain::registry.emplace<UnitEconomy::MetalCurrentMake>(entity);
        }
        if (!EcsMain::registry.all_of<UnitEconomyReport::SnapshotMetalMake>(entity)){
            EcsMain::registry.emplace<UnitEconomyReport::SnapshotMetalMake>(entity);
        }
    }
    else {
        EcsMain::registry.remove<UnitEconomy::MetalCurrentMake>(entity);
        EcsMain::registry.remove<UnitEconomyReport::SnapshotMetalMake>(entity);
    }
}

void UnitEconomyHelper::UpdateEconomyTrackMetalUse(entt::entity entity){
    if (EcsMain::registry.any_of<FlowEconomy::MetalFixedUse, FlowEconomy::MetalProratableUse>(entity)){
        if (!EcsMain::registry.all_of<UnitEconomy::MetalCurrentUsage>(entity)){
            EcsMain::registry.emplace<UnitEconomy::MetalCurrentUsage>(entity);
        }
        if (!EcsMain::registry.all_of<UnitEconomyReport::SnapshotMetalUsage>(entity)){
            EcsMain::registry.emplace<UnitEconomyReport::SnapshotMetalUsage>(entity);
        }
    }
    else {
        EcsMain::registry.remove<UnitEconomy::MetalCurrentUsage>(entity);
        EcsMain::registry.remove<UnitEconomyReport::SnapshotMetalUsage>(entity);
    }
}
