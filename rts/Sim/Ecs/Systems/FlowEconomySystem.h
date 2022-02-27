#ifndef FLOW_ECONOMY_SYSTEM_H__
#define FLOW_ECONOMY_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Ecs/SlowUpdate.h"
#include "Sim/Misc/Resource.h"

class CUnit;

class FlowEconomySystem {

public:
    void Init();
    void Update();

    void UpdateUnitProratableEnergyIncome(CUnit *unit, float amount);
    void UpdateUnitProratableMetalIncome(CUnit *unit, float amount);
    void UpdateUnitFixedEnergyIncome(CUnit *unit, float amount);
    void UpdateUnitFixedMetalIncome(CUnit *unit, float amount);

    void UpdateUnitProratableEnergyUse(CUnit *unit, float amount);
    void UpdateUnitProratableMetalUse(CUnit *unit, float amount);

    //void UpdateUnitProratableResourceUse(CUnit *unit, SResourcePack res);

    void UpdateUnitFixedEnergyIncome(entt::entity entity, float amount);

    bool IsSystemActive() { return active; }

private:
    bool active = false;

    void UpdateTeamEconomy(int teamId);
    // void UpdateWindGeneration();

    void UpdateFixedMetalIncome();
    void UpdateFixedEnergyIncome();

    void UpdateProratableMetalIncome();
    void UpdateProratableEnergyIncome();
    void UpdateProratableCombinedIncome();

    void UpdateWindGenerationIncome();

    void setEachBuildRate(int teamId, float minProrationRate, float energyProrationRate, float metalPropationRate);

    float economyMultiplier = 0.f;

    uint32_t slowUpdateIndexStart = 0;
    uint32_t lastFrameGroupSize = 0;
    uint32_t slowUpdateChunkSize = 0;

    SlowUpdateSubSystem fixedMetalIncomeUpdater;
    SlowUpdateSubSystem fixedEnergyIncomeUpdater;
    SlowUpdateSubSystem proratableMetalIncomeUpdater;
    SlowUpdateSubSystem proratableEnergyIncomeUpdater;
    SlowUpdateSubSystem proratableCombinedIncomeUpdater;
};

extern FlowEconomySystem flowEconomySystem;

#endif