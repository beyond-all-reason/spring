#ifndef UNIT_ECONOMY_HELPER_H__
#define UNIT_ECONOMY_HELPER_H__

#include "Sim/Ecs/EcsMain.h"

#include "Sim/Misc/Resource.h"

class CUnit;

class UnitEconomyHelper {
public:

    static void AddIncome(entt::entity entity, const SResourcePack& amount);
    static void AddUse(entt::entity entity, const SResourcePack& amount);

    static void RemoveIncome(entt::entity entity);
    static void RemoveUse(entt::entity entity);
};

#endif