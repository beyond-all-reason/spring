#ifndef UNIT_ECONOMY_REPORT_SYSTEM_H__
#define UNIT_ECONOMY_REPORT_SYSTEM_H__

#include "Sim/Ecs/EcsMain.h"
#include "Sim/Misc/Resource.h"

class CUnit;

class UnitEconomyReportSystem {

public:
    static void Init();
    static void Update();
};

#endif