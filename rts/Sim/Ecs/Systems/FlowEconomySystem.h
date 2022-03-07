#ifndef FLOW_ECONOMY_SYSTEM_H__
#define FLOW_ECONOMY_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Misc/Resource.h"

class CUnit;

class FlowEconomySystem {

public:
    void Init();
    void Update();

    void AddFlowEconomyUnit(CUnit *unit);

    void UpdateUnitProratableEnergyIncome(CUnit *unit, float amount);
    void UpdateUnitProratableMetalIncome(CUnit *unit, float amount);
    void UpdateUnitFixedEnergyIncome(CUnit *unit, float amount);
    void UpdateUnitFixedMetalIncome(CUnit *unit, float amount);

    void UpdateUnitProratableEnergyUse(CUnit *unit, float amount);
    void UpdateUnitProratableMetalUse(CUnit *unit, float amount);

    void UpdateUnitFixedEnergyIncome(entt::entity entity, float amount);

    bool RegisterOneOffExpense(CUnit* unit, float amount);

    bool IsSystemActive() { return active; }

private:
    bool active = false;

    void UpdateTeamEconomy(int teamId);
    // void UpdateWindGeneration();

    void UpdateFixedMetalIncome();
    void UpdateFixedEnergyIncome();

    void UpdateProratableMetalIncome();
    void UpdateProratableEnergyIncome();

    void UpdateFixedMetalExpense();
    void UpdateFixedEnergyExpense();

    void UpdateProratableMetalExpense();
    void UpdateProratableEnergyExpense();
    void UpdateProratableCombinedExpense();

    void SlowUpdate();

    float economyMultiplier = 0.f;
};

extern FlowEconomySystem flowEconomySystem;

#endif