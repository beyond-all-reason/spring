#include "UnitEconomyHelper.h"

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/FlowEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyComponents.h"
#include "Sim/Ecs/Components/UnitEconomyReportComponents.h"

#include "Sim/Units/Unit.h"

template<typename TF, typename... TR>
void AddComponentsIfNotExist(entt::entity entity) {
    if (!EcsMain::registry.all_of<TF>(entity)){
        EcsMain::registry.emplace<TF>(entity);
    }
    if constexpr (sizeof...(TR) > 0) {
        AddComponentsIfNotExist<TR...>(entity);
    }
}

template<typename... Components>
struct ComponentUpdateFuncs {
    void addComponentsIfNotExist(entt::entity entity) {
        AddComponentsIfNotExist<Components...>(entity);
    }

    void removeComponents(entt::entity entity) {
        EcsMain::registry.remove<Components...>(entity);
    }
};

template<class Component, typename... RelatedComponents, typename UpdateType>
void UpdateStuff(entt::entity entity, float amount, UpdateType& updater) {
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    if (amount <= 0.f){
        EcsMain::registry.remove<Component>(entity);
        if (!EcsMain::registry.all_of<RelatedComponents...>(entity)){
            updater.removeComponents(entity);
        }

        LOG("%s: %f [REMOVED]", typeid(Component).name(), amount);
    }
    else {
        EcsMain::registry.emplace_or_replace<Component>(entity, amount);
        updater.addComponentsIfNotExist(entity);

        LOG("%s: %f [UPDATED]", typeid(Component).name(), amount);
    }
}

void UnitEconomyHelper::UpdateUnitProratableEnergyIncome(CUnit *unit, float amount){
    auto entity = unit->entityReference;
    ComponentUpdateFuncs<UnitEconomy::EnergyCurrentMake, UnitEconomyReport::SnapshotEnergyMake> updater;
    UpdateStuff<FlowEconomy::EnergyProratableIncome, FlowEconomy::EnergyFixedIncome>(entity, amount, updater);
}

void UnitEconomyHelper::UpdateUnitProratableMetalIncome(CUnit *unit, float amount){
    auto entity = unit->entityReference;
    ComponentUpdateFuncs<UnitEconomy::MetalCurrentMake, UnitEconomyReport::SnapshotMetalMake> updater;
    UpdateStuff<FlowEconomy::MetalProratableIncome, FlowEconomy::MetalFixedIncome>(entity, amount, updater);
}

void UnitEconomyHelper::UpdateUnitFixedEnergyIncome(CUnit *unit, float amount){
    auto entity = unit->entityReference;
    ComponentUpdateFuncs<UnitEconomy::EnergyCurrentMake, UnitEconomyReport::SnapshotEnergyMake> updater;
    UpdateStuff<FlowEconomy::EnergyFixedIncome, FlowEconomy::EnergyProratableIncome>(entity, amount, updater);
}

void UnitEconomyHelper::UpdateUnitFixedMetalIncome(CUnit *unit, float amount) {
    auto entity = unit->entityReference;
    ComponentUpdateFuncs<UnitEconomy::MetalCurrentMake, UnitEconomyReport::SnapshotMetalMake> updater;
    UpdateStuff<FlowEconomy::MetalFixedIncome, FlowEconomy::MetalProratableIncome>(entity, amount, updater);
}

void UnitEconomyHelper::UpdateUnitProratableEnergyUse(CUnit *unit, float amount){
    auto entity = unit->entityReference;
    ComponentUpdateFuncs<UnitEconomy::EnergyCurrentUsage, UnitEconomyReport::SnapshotEnergyUsage> updater;
    UpdateStuff<FlowEconomy::EnergyProratableUse, FlowEconomy::EnergyFixedUse>(entity, amount, updater);
}

void UnitEconomyHelper::UpdateUnitProratableMetalUse(CUnit *unit, float amount){
    auto entity = unit->entityReference;
    ComponentUpdateFuncs<UnitEconomy::MetalCurrentUsage, UnitEconomyReport::SnapshotMetalUsage> updater;
    UpdateStuff<FlowEconomy::MetalProratableUse>(entity, amount, updater);
}

