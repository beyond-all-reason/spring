#ifndef ECONOMY_TASK_H__
#define ECONOMY_TASK_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/Components/UnitComponents.h"

class CUnit;

class EconomyTaskUtil {
public:
    static entt::entity CreateUnitEconomyTask(entt::entity unit);

    static bool DeleteUnitEconomyTask(entt::entity economyTask);

    static void DeleteAllUnitEconomyTasks(entt::entity unit);

    template<typename T>
    static void IterateAllUnitEconomyTasks(entt::entity unit, T func);
};

template<typename T>
void EconomyTaskUtil::IterateAllUnitEconomyTasks(entt::entity unit, T func) {
    auto headChainEntityComp = EcsMain::registry.try_get<Units::ChainEntity>(unit);
    if (headChainEntityComp == nullptr)
        return;

    for (auto nextInChain = headChainEntityComp->next; nextInChain != unit; ) {
        auto currentInChain = nextInChain;
        auto chainEntityComp = EcsMain::registry.try_get<Units::ChainEntity>(nextInChain);
        if (chainEntityComp == nullptr)
            nextInChain = unit;
        else
            nextInChain = chainEntityComp->next;
        
        func(currentInChain);
    }
}

#endif