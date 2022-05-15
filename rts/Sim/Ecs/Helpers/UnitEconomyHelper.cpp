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
void AddStuff(entt::entity entity, float amount, UpdateType& updater) {
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    EcsMain::registry.emplace_or_replace<Component>(entity, amount);
    updater.addComponentsIfNotExist(entity);

    LOG("%s: %f [UPDATED]", typeid(Component).name(), amount);
}

template<class Component, typename... RelatedComponents, typename UpdateType>
void RemoveStuff(entt::entity entity, UpdateType& updater) {
    if (! EcsMain::registry.valid(entity)){
        LOG("%s: invalid entityId reference", __func__); return;
    }

    EcsMain::registry.remove<Component>(entity);
    if (!EcsMain::registry.all_of<RelatedComponents...>(entity)){
        updater.removeComponents(entity);
    }

    LOG("%s: [REMOVED]", typeid(Component).name());
}

void UnitEconomyHelper::AddProratableEnergyIncome(entt::entity entity, float amount){
    ComponentUpdateFuncs<UnitEconomy::EnergyCurrentMake, UnitEconomyReport::SnapshotEnergyMake> updater;
    AddStuff<FlowEconomy::EnergyProratableIncome, FlowEconomy::EnergyFixedIncome>(entity, amount, updater);
}

void UnitEconomyHelper::AddProratableMetalIncome(entt::entity entity, float amount){
    ComponentUpdateFuncs<UnitEconomy::MetalCurrentMake, UnitEconomyReport::SnapshotMetalMake> updater;
    AddStuff<FlowEconomy::MetalProratableIncome, FlowEconomy::MetalFixedIncome>(entity, amount, updater);
}

void UnitEconomyHelper::AddFixedEnergyIncome(entt::entity entity, float amount){
    ComponentUpdateFuncs<UnitEconomy::EnergyCurrentMake, UnitEconomyReport::SnapshotEnergyMake> updater;
    AddStuff<FlowEconomy::EnergyFixedIncome, FlowEconomy::EnergyProratableIncome>(entity, amount, updater);
}

void UnitEconomyHelper::AddFixedMetalIncome(entt::entity entity, float amount) {
    ComponentUpdateFuncs<UnitEconomy::MetalCurrentMake, UnitEconomyReport::SnapshotMetalMake> updater;
    AddStuff<FlowEconomy::MetalFixedIncome, FlowEconomy::MetalProratableIncome>(entity, amount, updater);
}

void UnitEconomyHelper::AddProratableEnergyUse(entt::entity entity, float amount){
    ComponentUpdateFuncs<UnitEconomy::EnergyCurrentUsage, UnitEconomyReport::SnapshotEnergyUsage> updater;
    AddStuff<FlowEconomy::EnergyProratableUse>(entity, amount, updater);
}

void UnitEconomyHelper::AddProratableMetalUse(entt::entity entity, float amount){
    ComponentUpdateFuncs<UnitEconomy::MetalCurrentUsage, UnitEconomyReport::SnapshotMetalUsage> updater;
    AddStuff<FlowEconomy::MetalProratableUse>(entity, amount, updater);
}


void UnitEconomyHelper::RemoveProratableEnergyIncome(entt::entity entity){
    ComponentUpdateFuncs<UnitEconomy::EnergyCurrentMake, UnitEconomyReport::SnapshotEnergyMake> updater;
    RemoveStuff<FlowEconomy::EnergyProratableIncome, FlowEconomy::EnergyFixedIncome>(entity, updater);
}

void UnitEconomyHelper::RemoveProratableMetalIncome(entt::entity entity){
    ComponentUpdateFuncs<UnitEconomy::MetalCurrentMake, UnitEconomyReport::SnapshotMetalMake> updater;
    RemoveStuff<FlowEconomy::MetalProratableIncome, FlowEconomy::MetalFixedIncome>(entity, updater);
}

void UnitEconomyHelper::RemoveFixedEnergyIncome(entt::entity entity){
    ComponentUpdateFuncs<UnitEconomy::EnergyCurrentMake, UnitEconomyReport::SnapshotEnergyMake> updater;
    RemoveStuff<FlowEconomy::EnergyFixedIncome, FlowEconomy::EnergyProratableIncome>(entity, updater);
}

void UnitEconomyHelper::RemoveFixedMetalIncome(entt::entity entity) {
    ComponentUpdateFuncs<UnitEconomy::MetalCurrentMake, UnitEconomyReport::SnapshotMetalMake> updater;
    RemoveStuff<FlowEconomy::MetalFixedIncome, FlowEconomy::MetalProratableIncome>(entity, updater);
}

void UnitEconomyHelper::RemoveProratableEnergyUse(entt::entity entity){
    ComponentUpdateFuncs<UnitEconomy::EnergyCurrentUsage, UnitEconomyReport::SnapshotEnergyUsage> updater;
    RemoveStuff<FlowEconomy::EnergyProratableUse>(entity, updater);
}

void UnitEconomyHelper::RemoveProratableMetalUse(entt::entity entity){
    ComponentUpdateFuncs<UnitEconomy::MetalCurrentUsage, UnitEconomyReport::SnapshotMetalUsage> updater;
    RemoveStuff<FlowEconomy::MetalProratableUse>(entity, updater);
}