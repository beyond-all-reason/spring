#ifndef FLOW_ECONOMY_SYSTEM_H__
#define FLOW_ECONOMY_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Misc/Resource.h"

class CUnit;

class FlowEconomySystem {

public:
    void Init();
    void Update();

    void AddUnitEconomy(CUnit *unit);

    void UpdateUnitProratableEnergyCreation(CUnit *unit, float amount);
    void UpdateUnitProratableMetalCreation(CUnit *unit, float amount);
    void UpdateUnitFixedEnergyCreation(CUnit *unit, float amount);
    void UpdateUnitFixedMetalCreation(CUnit *unit, float amount);
    void UpdateUnitProratableResourceUse(CUnit *unit, SResourcePack res);

    void UpdateUnitFixedEnergyCreation(entt::entity entity, float amount);

    bool IsSystemActive() { return active; }

private:
    bool active = false;

    void UpdateTeamEconomy(int teamId);

    void setEachBuildRate(int teamId, float minProrationRate, float energyProrationRate, float metalPropationRate);

    float economyMultiplier = 0.f;
};

extern FlowEconomySystem flowEconomySystem;

#endif