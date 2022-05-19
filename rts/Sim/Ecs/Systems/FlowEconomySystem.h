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
    void UpdateTeamEconomy(int teamId);

    void ProcessProratableIncome();
    void ProcessFixedIncome();
    void ProcessExpenses();

    float economyMultiplier = 0.f;
};

extern FlowEconomySystem flowEconomySystem;

#endif