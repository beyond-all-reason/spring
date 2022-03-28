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

    bool IsSystemActive() { return active; }

private:
    bool active = false;


    void UpdateEconomyPredictions();
    void UpdateAllTeamsEconomy();
    void InformWaitingEntitiesEconomyIsAssigned();
    void UpdateTeamEconomy(int teamId);

    void UpdateFixedMetalIncome();
    void UpdateFixedEnergyIncome();

    void UpdateProratableMetalIncome();
    void UpdateProratableEnergyIncome();

    void UpdateFixedMetalExpense();
    void UpdateFixedEnergyExpense();

    void UpdateProratableMetalExpense();
    void UpdateProratableEnergyExpense();
    void UpdateProratableCombinedExpense();

    float economyMultiplier = 0.f;
};

extern FlowEconomySystem flowEconomySystem;

#endif