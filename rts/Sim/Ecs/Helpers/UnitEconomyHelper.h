#ifndef UNIT_ECONOMY_HELPER_H__
#define UNIT_ECONOMY_HELPER_H__

#include "Sim/Ecs/EcsMain.h"

class CUnit;

class UnitEconomyHelper {
public:

    static void AddProratableEnergyIncome(entt::entity entity, float amount);
    static void AddProratableMetalIncome(entt::entity entity, float amount);
    static void AddFixedEnergyIncome(entt::entity entity, float amount);
    static void AddFixedMetalIncome(entt::entity entity, float amount);
    static void AddProratableEnergyUse(entt::entity entity, float amount);
    static void AddProratableMetalUse(entt::entity entity, float amount);

    static void RemoveProratableEnergyIncome(entt::entity entity);
    static void RemoveProratableMetalIncome(entt::entity entity);
    static void RemoveFixedEnergyIncome(entt::entity entity);
    static void RemoveFixedMetalIncome(entt::entity entity);
    static void RemoveProratableEnergyUse(entt::entity entity);
    static void RemoveProratableMetalUse(entt::entity entity);

    static void UpdateEconomyTrackEnergyMake(entt::entity entity);
    static void UpdateEconomyTrackEnergyUse(entt::entity entity);
    static void UpdateEconomyTrackMetalMake(entt::entity entity);
    static void UpdateEconomyTrackMetalUse(entt::entity entity);
};

#endif