#ifndef UNIT_ECONOMY_SYSTEM_H__
#define UNIT_ECONOMY_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Misc/Resource.h"

class CUnit;

class UnitEconomySystem {

public:
    void Init();
    void Update();

    bool IsSystemActive() { return active; }

private:
    bool active = false;

    float economyMultiplier = 0.f;

    void UpdateEnergyIncomeTracking();
    void UpdateMetalIncomeTracking();
    void UpdateEnergyUsageTracking();
    void UpdateMetalUsageTracking();
    void UpdateEconomyCombinedUsageTracking();
};

extern UnitEconomySystem unitEconomySystem;

#endif