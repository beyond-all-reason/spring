#ifndef UNIT_ECONOMY_HELPER_H__
#define UNIT_ECONOMY_HELPER_H__

#include "Sim/Ecs/EcsMain.h"

class CUnit;

class UnitEconomyHelper {
public:
    static void UpdateUnitProratableEnergyIncome(CUnit *unit, float amount);
    static void UpdateUnitProratableMetalIncome(CUnit *unit, float amount);
    static void UpdateUnitFixedEnergyIncome(CUnit *unit, float amount);
    static void UpdateUnitFixedMetalIncome(CUnit *unit, float amount);
    static void UpdateUnitFixedEnergyExpense(CUnit *unit, float amount);
    static void UpdateUnitFixedMetalExpense(CUnit *unit, float amount);
    static void UpdateUnitProratableEnergyUse(CUnit *unit, float amount);
    static void UpdateUnitProratableMetalUse(CUnit *unit, float amount);

    static void UpdateEconomyTrackEnergyMake(entt::entity entity);
    static void UpdateEconomyTrackEnergyUse(entt::entity entity);

    static void UpdateEconomyTrackMetalMake(entt::entity entity);
    static void UpdateEconomyTrackMetalUse(entt::entity entity);
};

#endif