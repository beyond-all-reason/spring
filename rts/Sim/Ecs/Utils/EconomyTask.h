#ifndef ECONOMY_TASK_H__
#define ECONOMY_TASK_H__

#include "Sim/Ecs/EcsMain.h"

class CUnit;

class EconomyTaskUtil {
public:
    static entt::entity CreateUnitEconomyTask(entt::entity unit);

    static bool DeleteUnitEconomyTask(entt::entity economyTask);
};

#endif