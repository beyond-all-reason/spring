#ifndef UNIT_ECONOMY_REPORT_SYSTEM_H__
#define UNIT_ECONOMY_REPORT_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Misc/Resource.h"

class CUnit;

class UnitEconomyReportSystem {

public:
    void Init();
    void Update();

    bool IsSystemActive() { return active; }

private:
    bool active = false;

    float economyMultiplier = 0.f;

    void TakeMakeSnapshot();
    void TakeUseSnapshot();
};

extern UnitEconomyReportSystem unitEconomyReportSystem;

#endif