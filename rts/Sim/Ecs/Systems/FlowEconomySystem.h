#ifndef FLOW_ECONOMY_SYSTEM_H__
#define FLOW_ECONOMY_SYSTEM_H__

class CUnit;

class FlowEconomySystem {

public:
    void Init();
    void Update();

    void AddUnitEconomy(CUnit *unit);
private:
    bool active = false;

    void UpdateTeamEconomy(int teamId);

    void setEachBuildRate(int teamId, float minProrationRate, float energyProrationRate, float metalPropationRate);

    float getUnconditionalEnergyUse(int teamId);
    float getUnconditionalMetalUse(int teamId);

    float getPoratableEnergyUse(int teamId);
    float getPoratableMetalUse(int teamId);

    float getRecalculatedEnergyUse(int teamId, float energyRate, float metalRate);
    float getRecalculatedMetalUse(int teamId, float energyRate, float metalRate);

    float prorateEnergyUse(int teamId, float prorationRate);
    float propateMetalUse(int teamId, float prorationRate);

    float getTotalEnergyIncome(int teamId, float prorationRate);
    float getTotalMetalIncome(int teamId, float prorationRate);

    float economyMultiplier = 0.f;
};

extern FlowEconomySystem flowEconomySystem;

#endif