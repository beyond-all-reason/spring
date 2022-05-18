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

template<class Component, typename... RelatedComponents, typename UpdateType, typename T>
void AddStuff(entt::entity entity, T amount, UpdateType& updater) {
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

void UnitEconomyHelper::AddIncome(entt::entity entity, const SResourcePack& amount){
    ComponentUpdateFuncs<UnitEconomy::ResourcesCurrentMake, UnitEconomyReport::SnapshotMake> updater;
    AddStuff<FlowEconomy::ResourceAdd>(entity, amount, updater);
}

void UnitEconomyHelper::AddUse(entt::entity entity, const SResourcePack& amount){
    ComponentUpdateFuncs<UnitEconomy::ResourcesCurrentUsage, UnitEconomyReport::SnapshotUsage> updater;
    AddStuff<FlowEconomy::ResourceUse>(entity, amount, updater);
}

void UnitEconomyHelper::RemoveIncome(entt::entity entity){
    ComponentUpdateFuncs<UnitEconomy::ResourcesCurrentMake, UnitEconomyReport::SnapshotMake> updater;
    RemoveStuff<FlowEconomy::ResourceAdd>(entity, updater);
}

void UnitEconomyHelper::RemoveUse(entt::entity entity){
    ComponentUpdateFuncs<UnitEconomy::ResourcesCurrentUsage, UnitEconomyReport::SnapshotUsage> updater;
    RemoveStuff<FlowEconomy::ResourceUse>(entity, updater);
}
